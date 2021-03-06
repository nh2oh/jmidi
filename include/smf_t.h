#pragma once
#include "generic_chunk_low_level.h"  // is_mthd_header_id()
#include "mthd_t.h"
#include "mtrk_t.h"
#include "generic_iterator.h"
#include <string>
#include <cstdint>
#include <vector>
#include <filesystem>


namespace jmid {

struct smf_error_t;
class smf_t;

//
// smf_t
//
// Holds an smf, presenting an interface similar to a std::vector<mtrk_t>.  
// Also stores the MThd chunk, and non-MTrk non-MThd "unknown" chunks 
// ("uchk"s as a std::vector<std::vector<unsigned char>>.  The relative 
// order of the MTrk and uchks as they appeared in the file is preserved.  
//
// Although both MTrk and "unknown" are stored in an smf_t, the STL 
// container-inspired methods size(), operator[], begin(), end(), etc, 
// only report on and return accessors to the MTrk chunks.  Thus, a 
// range-for loop over an smf_t, ex:
// for (const auto& chunk : my_smf_t) { //...
// only loops over the MTrk chunks; the size() method returns the number
// of MTrks and ignores the unknown chunks.  A seperate set of methods
// (nuchks(), get_uchk(int32_t idx), etc) access the unknown chunks.  
// Methods such as insert(), erase() etc are overloaded on 
// std::vector<std::vector<unsigned char>>::[const_]iterator and provide
// access to the uchks.  
//
// Invariants:
// The num-tracks field in the member MThd chunk is kept consistent with 
// the number of MTrks held by the container.  
// 
// TODO:  verify()
// TODO:  Ctors
// TODO:  No nticks() (see mtrk_t)
//

struct smf_container_types_t {
	using value_type = jmid::mtrk_t;
	using size_type = std::int64_t;
	using difference_type = std::int32_t; //std::ptrdiff_t;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;
};

class smf_t {
public:
	using value_type = smf_container_types_t::value_type;
	using size_type = smf_container_types_t::size_type;
	using difference_type = smf_container_types_t::difference_type;
	using reference = smf_container_types_t::reference;
	using const_reference = smf_container_types_t::const_reference;
	using pointer = smf_container_types_t::pointer;
	using const_pointer = smf_container_types_t::const_pointer;
	using iterator = internal::generic_ra_iterator<smf_container_types_t>;
	using const_iterator = internal::generic_ra_const_iterator<smf_container_types_t>;

	using uchk_iterator = std::vector<std::vector<unsigned char>>::iterator;
	using uchk_const_iterator = std::vector<std::vector<unsigned char>>::const_iterator;
	using uchk_value_type = std::vector<unsigned char>;

	smf_t() noexcept;
	smf_t(const smf_t&);
	smf_t(smf_t&&) noexcept;
	smf_t& operator=(const smf_t&);
	smf_t& operator=(smf_t&&) noexcept;
	~smf_t() noexcept;

	size_type size() const;  // Number of mtrk chunks
	size_type nchunks() const;  // Number of MTrk + Unkn chunks
	size_type ntrks() const;  // Number of MTrk chunks
	size_type nuchks() const;  // Number of MTrk chunks
	size_type nbytes() const;  // Number of bytes serialized

	iterator begin();
	iterator end();
	const_iterator cbegin() const;
	const_iterator cend() const;
	const_iterator begin() const;
	const_iterator end() const;
	
	reference push_back(const_reference);
	reference push_back(jmid::mtrk_t&&);
	iterator insert(iterator, const_reference);
	const_iterator insert(const_iterator, const_reference);
	iterator erase(iterator);
	const_iterator erase(const_iterator);

	const uchk_value_type& push_back(const uchk_value_type&);
	uchk_iterator insert(uchk_iterator, const uchk_value_type&);
	uchk_const_iterator insert(uchk_const_iterator, const uchk_value_type&);
	uchk_iterator erase(uchk_iterator);
	uchk_const_iterator erase(uchk_const_iterator);

	reference operator[](size_type);
	const_reference operator[](size_type) const;
	const uchk_value_type& get_uchk(size_type) const;
	uchk_value_type& get_uchk(size_type);

	//
	// MThd accessors
	//
	const jmid::mthd_t& mthd() const;
	jmid::mthd_t& mthd();
	std::int32_t format() const;  // mthd alias
	jmid::time_division_t division() const;  // mthd alias
	std::int32_t mthd_size() const;  // mthd alias
	void set_mthd(const jmid::maybe_mthd_t&);
	void set_mthd(const jmid::mthd_t&);
	void set_mthd(jmid::mthd_t&&) noexcept;
private:
	jmid::mthd_t mthd_;
	std::vector<value_type> mtrks_;
	std::vector<std::vector<unsigned char>> uchks_ {};
	// Since MTrk and unknown chunks are split into mtrks_ and uchks_,
	// respectively, the order in which these chunks occured in the file
	// is lost.  chunkorder_ saves the order of interspersed MTrk and 
	// unknown chunks.  chunkorder.size()==mtrks_.size+uchks.size().  
	std::vector<int> chunkorder_ {};  // 0=>mtrk, 1=>unknown

	template<typename InIt>
	friend InIt make_smf2(InIt, InIt, smf_t*, smf_error_t*);
};
std::string print(const smf_t&);

struct smf_error_t {
	enum class errc : std::uint8_t {
		file_read_error,
		mthd_error,
		mtrk_error,
		overflow_reading_uchk,
		terminated_before_end_of_file,  // TODO:  Nothing sets this
		unexpected_num_mtrks,
		no_error,
		other
	};
	jmid::mthd_error_t mthd_err_obj;
	jmid::mtrk_error_t mtrk_err_obj;
	uint16_t expect_num_mtrks;
	uint16_t num_mtrks_read;
	uint16_t num_uchks_read;
	smf_error_t::errc code;
};
struct maybe_smf_t {
	smf_t smf;
	std::ptrdiff_t nbytes_read;
	smf_error_t::errc error;
	operator bool() const;
};
//
// maybe_smf_t read_smf(...)
//
// (1)
// maybe_smf_t read_smf(const std::filesystem::path&, smf_error_t*);
// Constructs a std::basic_ifstream<char> and std::istreambuf_iterator<char>
// iterator pair, then delegates to make_smf().  
// 
// (2)
// maybe_smf_t read_smf_bulkfileread(const std::filesystem::path&,
//									smf_error_t*, std::vector<char>*);
// Constructs a std::basic_ifstream<char> and std::istreambuf_iterator<char>
// iterator pair, then copies the entire file into a 
// std::vector<char>, in one shot w/a call to std::ifstream::read().  
// Delegates to make_smf() w/ the vector iterators.
//
maybe_smf_t read_smf(const std::filesystem::path&, smf_error_t*, std::int32_t);
maybe_smf_t read_smf_bulkfileread(const std::filesystem::path&, 
						smf_error_t*, std::vector<char>*, std::int32_t);
// std::string print(smf_error_t::errc ec);
// if ec == smf_error_t::errc::no_error, returns an empty string
std::string print(smf_error_t::errc);
std::string explain(const smf_error_t&);


//
// Overwrites the smf object at result with the contents read in from 
// [it,end).  The mthd and mtrks arrays are resized to reflect the actual
// sizes of those elements in the input.  
//
template<typename InIt>
InIt make_smf2(InIt it, InIt end, smf_t *result, smf_error_t *err) {
	auto set_error = [&result,&err](smf_error_t::errc ec, int expect_ntrks, 
						int n_mtrks_read, int n_uchks_read)->void {
		result->mtrks_.resize(n_mtrks_read);
		result->uchks_.resize(n_uchks_read);
		if (err!=nullptr) {
			err->code = ec;
			err->num_mtrks_read = n_mtrks_read;
			err->num_uchks_read = n_uchks_read;
			err->expect_num_mtrks = expect_ntrks;
		}
	};

	jmid::mthd_error_t mthd_err;
	it = jmid::make_mthd2(it,end,&(result->mthd_),&mthd_err);  // TODO:  Temporary 14
	if (mthd_err.code != mthd_error_t::errc::no_error) {
		set_error(smf_error_t::errc::mthd_error,0,0,0);
		return it;
	}
	// NB: Calling result->set_mthd(some_mthd_object) cause the ntrks field
	// in the mthd to be modified to match result->mtrks_.size(), which is
	// presently 0.  

	auto expect_ntrks = result->mthd_.ntrks();
	int n_mtrks_read = 0;
	int n_uchks_read = 0;
	result->chunkorder_.resize(0);
	while ((it!=end) && (n_mtrks_read<expect_ntrks)) {
		jmid::chunk_header_t curr_chk_header;
		jmid::chunk_header_error_t curr_chk_header_err;
		it = jmid::read_chunk_header(it,end,&curr_chk_header,
			&curr_chk_header_err);
		if (curr_chk_header_err.code != jmid::chunk_header_error_t::errc::no_error) {
			set_error(smf_error_t::errc::other,0,0,0);
			return it;
		}

		if (jmid::has_mtrk_id(curr_chk_header) 
				&& jmid::has_valid_length(curr_chk_header)) {
			if (result->mtrks_.size() == n_mtrks_read) {
				result->mtrks_.resize(result->mtrks_.size()+1);
				result->mtrks_.back().resize(2000);  // TODO:  Magic number 2000
				// NB:  If not calling push_back(), the smf_t will not 
				// correctly record the uchk-mtrk sequence order
			}
			auto p_curr_mtrk = result->mtrks_.data() + n_mtrks_read;
			jmid::mtrk_error_t curr_mtrk_error;
			it = jmid::make_mtrk_event_seq(it,end,0x00u,p_curr_mtrk,&curr_mtrk_error);
			if (curr_mtrk_error.code != jmid::mtrk_error_t::errc::no_error) {
				// Invalid MTrk
				set_error(smf_error_t::errc::mtrk_error,expect_ntrks,
					n_mtrks_read,n_uchks_read);
				return it;
			}
			result->chunkorder_.push_back(0);
			++n_mtrks_read;
		} else if (jmid::has_uchk_id(curr_chk_header) 
				&& jmid::has_valid_length(curr_chk_header)) {
			if (result->uchks_.size() == n_uchks_read) {
				result->uchks_.resize(result->uchks_.size()+1);
				result->uchks_.back().reserve(2000);  // TODO:  Magic number 2000
			}
			auto bi_uchk = std::back_inserter(result->uchks_[n_uchks_read]);
			std::uint32_t j=0;
			for (j=0; ((it!=end) && (j<curr_chk_header.length)); ++j) {
				*bi_uchk++ = static_cast<unsigned char>(*it++);
			}
			if (j!=curr_chk_header.length) {
				// Invalid UChk
				set_error(smf_error_t::errc::overflow_reading_uchk,
							expect_ntrks,n_mtrks_read,n_uchks_read);
				return it;
			}
			result->chunkorder_.push_back(1);
			++n_uchks_read;
		} else {  // Non-MTrk, non-UChk header field
			set_error(smf_error_t::errc::other,expect_ntrks,n_mtrks_read,
					n_uchks_read);
			return it;
		}
	}  // To next chunk

	if (n_mtrks_read != expect_ntrks) {
		set_error(smf_error_t::errc::unexpected_num_mtrks,
				expect_ntrks,n_mtrks_read,n_uchks_read);
		return it;
	}
	
	set_error(smf_error_t::errc::no_error,expect_ntrks,n_mtrks_read,
				n_uchks_read);
	return it;
};


/*
template<typename InIt>
InIt make_smf(InIt it, InIt end, maybe_smf_t *result, smf_error_t *err,
			const std::int32_t max_stream_bytes) {
	jmid::mtrk_error_t curr_mtrk_error;  // ... ???
	jmid::mthd_error_t* p_mthd_error = nullptr;
	if (err) {
		p_mthd_error = &(err->mthd_err_obj);
	}
	std::ptrdiff_t i = 0;  // The number of bytes read from the stream

	auto set_error = [&result,&err,&i,&curr_mtrk_error]
					(smf_error_t::errc ec, int expect_ntrks, 
						int n_mtrks_read, int n_uchks_read)->void {
		result->nbytes_read = i;
		result->error = ec;
		if (err) {
			err->code = ec;
			err->mtrk_err_obj = curr_mtrk_error;
			err->num_mtrks_read = n_mtrks_read;
			err->num_uchks_read = n_uchks_read;
			err->expect_num_mtrks = expect_ntrks;
		}
	};

	// TODO:  Totally defeats the point of having the caller 
	// prereserve & pass in a ptr
	jmid::maybe_mthd_t maybe_mthd;
	it = jmid::make_mthd(it,end,&maybe_mthd,p_mthd_error,max_stream_bytes);
	i += maybe_mthd.nbytes_read;
	if (!maybe_mthd) {
		set_error(smf_error_t::errc::mthd_error,0,0,0);
		return it;
	}
	auto expect_ntrks = maybe_mthd.mthd.ntrks();
	// When set_mthd() is called, the smf_t object will modify the 
	// ntrks field in the mthd_t to match its member mtrks_.size(),
	// which is presently 0.  
	result->smf.set_mthd(std::move(maybe_mthd.mthd));
	// auto expect_ntrks = result->smf.mthd().ntrks();

	int n_mtrks_read = 0;
	int n_uchks_read = 0;
	// TODO:  Totally defeats the point of having the caller 
	// prereserve & pass in a ptr
	jmid::maybe_mtrk_t curr_mtrk;
	while ((it!=end) && (n_mtrks_read<expect_ntrks) && (i<max_stream_bytes)) {
		it = jmid::make_mtrk(it,end,&curr_mtrk,&curr_mtrk_error,max_stream_bytes-i);
		i += curr_mtrk.nbytes_read;

		// If it was pointing at the first byte of a UChk header...
		if (!curr_mtrk && (curr_mtrk.error == jmid::mtrk_error_t::errc::valid_but_non_mtrk_id)) {
			auto ph = curr_mtrk_error.header.data();
			auto hsz = curr_mtrk_error.header.size();
			if (!jmid::is_mthd_header_id(ph,ph+hsz)) {
				std::vector<unsigned char> curr_uchk;
				auto bi_uchk = std::back_inserter(curr_uchk);
				std::copy(ph,ph+hsz,bi_uchk);
				auto uchk_sz = jmid::read_be<uint32_t>(ph+4,ph+hsz);
				std::uint32_t j=0;
				for (j=0; ((it!=end) && (j<uchk_sz) && (i<max_stream_bytes)); ++j) {
					*bi_uchk++ = static_cast<unsigned char>(*it++);  ++i;
				}
				if (j!=uchk_sz) {
					set_error(smf_error_t::errc::overflow_reading_uchk,
						expect_ntrks,n_mtrks_read,n_uchks_read);
					return it;
				}
				++n_uchks_read;
				result->smf.push_back(curr_uchk);
				continue;
			}
		}

		// Here, curr_mtrk is either valid or (is invalid _and_ is not 
		// a UChk).  
		// push_back the mtrk even if invalid; make_mtrk will return a
		// partial mtrk terminating at the event right before the error,
		// and this partial mtrk may be useful to the user.  
		result->smf.push_back(std::move(curr_mtrk.mtrk));
		curr_mtrk.mtrk.clear();

		if (!curr_mtrk) {
			// Invalid as an MTrk, but not for reason of being a UChk
			set_error(smf_error_t::errc::mtrk_error,expect_ntrks,
				n_mtrks_read,n_uchks_read);
			return it;
		}
		++n_mtrks_read;
	}

	if (n_mtrks_read != expect_ntrks) {
		if (i >= max_stream_bytes) {
			set_error(smf_error_t::errc::other,  // TODO:  Wrong error code
				expect_ntrks,n_mtrks_read,n_uchks_read);
			return it;
		}
		set_error(smf_error_t::errc::unexpected_num_mtrks,
				expect_ntrks,n_mtrks_read,n_uchks_read);
		return it;
	}
	
	result->error = smf_error_t::errc::no_error;
	result->nbytes_read = i;
	return it;
};*/


template<typename OIt>
OIt write_smf(const smf_t& smf, OIt it) {
	for (const auto& e : smf.mthd()) {
		*it++ = e;
	}
	for (const auto& trk : smf) {
		it = write_mtrk(trk,it);
	}
	return it;
};

std::filesystem::path write_smf(const smf_t&, const std::filesystem::path&);

// For the path fp, checks that std::filesystem::is_regular_file(fp)
// is true, and that the extension is "mid", "MID", "midi", or 
// "MIDI"
bool has_midifile_extension(const std::filesystem::path&);

/*
struct mtrk_event_range_t {
	mtrk_event_t::const_iterator beg;
	mtrk_event_t::const_iterator end;
};
class sequential_range_iterator {
public:
private:
	struct range_pos {
		mtrk_event_t::const_iterator beg;
		mtrk_event_t::const_iterator end;
		mtrk_event_t::const_iterator curr;
		std::uint32_t tkonset;
	};
	std::vector<range_pos> r_;
	std::vector<range_pos>::iterator curr_;
	std::uint32_t tkonset_;
};
class simultaneous_range_iterator {
public:
private:
	struct range_pos {
		mtrk_event_t::const_iterator beg;
		mtrk_event_t::const_iterator end;
		mtrk_event_t::const_iterator curr;
		std::uint32_t tkonset;
	};
	std::vector<range_pos> r_;
	std::vector<range_pos>::iterator curr_;
	std::uint32_t tkonset_;
};
*/

/*class smf2_chrono_iterator {
public:
private:
	struct event_range_t {
		mtrk_iterator_t beg;
		mtrk_iterator_t end;
	};
	std::vector<event_range_t> trks_;
};*/


struct all_smf_events_dt_ordered_t {
	mtrk_event_t ev;
	std::uint32_t cumtk;
	int trackn;
};
std::vector<all_smf_events_dt_ordered_t> get_events_dt_ordered(const smf_t&);
std::string print(const std::vector<all_smf_events_dt_ordered_t>&);

/*
struct linked_pair_with_trackn_t {
	int trackn;
	linked_onoff_pair_t ev_pair;
};
struct orphan_onoff_with_trackn_t {
	int trackn;
	orphan_onoff_t orph_ev;
};
struct linked_and_orphans_with_trackn_t {
	std::vector<linked_pair_with_trackn_t> linked {};
	std::vector<orphan_onoff_with_trackn_t> orphan_on {};
	std::vector<orphan_onoff_with_trackn_t> orphan_off {};
};
linked_and_orphans_with_trackn_t get_linked_onoff_pairs(const smf_t&);
std::string print(const linked_and_orphans_with_trackn_t&);
*/




/*
struct smf_simultaneous_event_range_t {
	std::vector<simultaneous_event_range_t> trks;
};
smf_simultaneous_event_range_t 
make_smf_simultaneous_event_range(mtrk_iterator_t beg, mtrk_iterator_t end);
*/


}  // namespace jmid
