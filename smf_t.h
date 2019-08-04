#pragma once
#include "generic_chunk_low_level.h"  // chunk_header_error_t in smf_error_t
#include "mthd_t.h"
#include "mtrk_t.h"
#include "..\..\generic_iterator.h"
#include <string>
#include <cstdint>
#include <vector>
#include <filesystem>

//
// smf_t
//
// Holds an smf, presenting an interface similar to a std::vector<mtrk_t>.  
// Also stores the MThd chunk, and non-MTrk non-MThd "unknown" chunks 
// ("uchk"s as a std::vector<std::vector<unsigned char>>.  The relative 
// order of the MTrk and uchks as they appeared in the file is preserved.  
//
// Although both MTrk and  "unknown" are stored in an smf_t, the STL 
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
//
//
// Invariants:
// Values of the MThd chunk parameters and the number of MTrk and Uchk 
// chunks (ex mthd_.format()==0 => .ntrks()==1) is maintained.  
// 
// TODO:  verify()
// TODO:  Ctors
// TODO:  insert(), erase() etc should edit the MThd
// TODO:  Does not really need to hold an mthd_t; can just hold 3 
// uint16_t's representing the data fields.  Saves 24-3*2 == 18
// bytes...
//
//

struct smf_container_types_t {
	using value_type = mtrk_t;
	using size_type = int64_t;
	using difference_type = std::ptrdiff_t;
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
	using iterator = generic_ra_iterator<smf_container_types_t>;
	using const_iterator = generic_ra_const_iterator<smf_container_types_t>;

	using uchk_iterator = std::vector<std::vector<unsigned char>>::iterator;
	using uchk_const_iterator = std::vector<std::vector<unsigned char>>::const_iterator;
	using uchk_value_type = std::vector<unsigned char>;

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
	const mthd_t& mthd() const;
	mthd_t& mthd();
	int32_t format() const;  // mthd alias
	time_division_t division() const;  // mthd alias
	int32_t mthd_size() const;  // mthd alias
	void set_mthd(const maybe_mthd_t&);
private:
	mthd_t mthd_ {};
	std::vector<value_type> mtrks_ {};
	std::vector<std::vector<unsigned char>> uchks_ {};
	// Since MTrk and unknown chunks are split into mtrks_ and uchks_,
	// respectively, the order in which these chunks occured in the file
	// is lost.  chunkorder_ saves the order of interspersed MTrk and 
	// unknown chunks.  chunkorder.size()==mtrks_.size+uchks.size().  
	std::vector<int> chunkorder_ {};  // 0=>mtrk, 1=>unknown
};
std::string print(const smf_t&);

struct smf_error_t {
	enum class errc : uint8_t {
		file_read_error,
		mthd_error,
		mtrk_error,
		overflow_reading_uchk,
		terminated_before_end_of_file,
		unexpected_num_mtrks,
		no_error,
		other
	};
	mthd_error_t mthd_err_obj;
	mtrk_error_t mtrk_err_obj;
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
maybe_smf_t read_smf(const std::filesystem::path&, smf_error_t*);
maybe_smf_t read_smf_bulkfileread(const std::filesystem::path&, smf_error_t*);
std::string explain(const smf_error_t&);

template<typename InIt>
InIt make_smf(InIt it, InIt end, maybe_smf_t *result, smf_error_t *err) {
	mtrk_error_t curr_mtrk_error;
	mthd_error_t* p_mthd_error = nullptr;
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

	maybe_mthd_t maybe_mthd;
	it = make_mthd(it,end,&maybe_mthd,p_mthd_error);
	i += maybe_mthd.nbytes_read;
	if (!maybe_mthd) {
		set_error(smf_error_t::errc::mthd_error,0,0,0);
		return it;
	}
	result->smf.set_mthd(maybe_mthd);
	
	auto expect_ntrks = result->smf.mthd().ntrks();
	int n_mtrks_read = 0;
	int n_uchks_read = 0;
	maybe_mtrk_t curr_mtrk;  
	while ((it!=end) && (n_mtrks_read<expect_ntrks)) {
		it = make_mtrk(it,end,&curr_mtrk,&curr_mtrk_error);
		i += curr_mtrk.nbytes_read;

		// If it was pointing at the first byte of a UChk header...
		if (!curr_mtrk && (curr_mtrk.error == mtrk_error_t::errc::valid_but_non_mtrk_id)) {
			auto ph = curr_mtrk_error.header.data();
			auto hsz = curr_mtrk_error.header.size();
			if (!is_mthd_header_id(ph,ph+hsz)) {
				std::vector<unsigned char> curr_uchk;
				auto bi_uchk = std::back_inserter(curr_uchk);
				std::copy(ph,ph+hsz,bi_uchk);
				auto uchk_sz = read_be<uint32_t>(ph+4,ph+hsz);
				uint32_t j=0;
				for (j=0; (it!=end && j<uchk_sz); ++j) {
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
		set_error(smf_error_t::errc::unexpected_num_mtrks,
				expect_ntrks,n_mtrks_read,n_uchks_read);
		return it;
	}
	
	result->error = smf_error_t::errc::no_error;
	result->nbytes_read = i;
	return it;
};


// For the path fp, checks that std::filesystem::is_regular_file(fp)
// is true, and that the extension is "mid", "MID", "midi", or 
// "MIDI"
bool has_midifile_extension(const std::filesystem::path&);


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
		uint32_t tkonset;
	};
	std::vector<range_pos> r_;
	std::vector<range_pos>::iterator curr_;
	uint32_t tkonset_;
};
class simultaneous_range_iterator {
public:
private:
	struct range_pos {
		mtrk_event_t::const_iterator beg;
		mtrk_event_t::const_iterator end;
		mtrk_event_t::const_iterator curr;
		uint32_t tkonset;
	};
	std::vector<range_pos> r_;
	std::vector<range_pos>::iterator curr_;
	uint32_t tkonset_;
};

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
	uint32_t cumtk;
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


