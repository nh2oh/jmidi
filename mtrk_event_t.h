#pragma once
#include "midi_raw.h"  // declares smf_event_type
#include <string>
#include <cstdint>
#include <array>


class mtrk_event_t;
class mtrk_event_iterator_t;
class mtrk_event_const_iterator_t;

enum class mtrk_sbo_print_opts {
	normal,
	detail,
	// Prints the value of flags_, midi_status_.  If big, prints the byte 
	// array d_ for the local container in addition to the heap-allocated
	// data.  
	debug  
};
std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts=mtrk_sbo_print_opts::normal);

//
// mtrk_event_t:  An sbo-featured container for mtrk events
//
// A side effect of always storing the midi-status byte applic. to the event
// in midi_status_ is that running-status events extracted from a file can 
// be stored elsewhere and interpreted correctly later.  For example, in 
// linking note pairs, pairs of corresponding on and off events are collected
// into a vector of linked events.  
//
// TODO:  
// If midi_status_ held the first byte following the delta-time in all cases
// (including for sysex,meta,etc events), it would be _way_ more effecient to
// determine the object type dynamically.  One would not have to process the
// delta-time field.  Presumably .type() is the most common method called on
// these objects.  
//
// TODO:  This exposes a lot of dangerous getters
// TODO:  ==,!= operators
// TODO:  Member enum class offs is absolutely disgusting
//
class mtrk_event_t {
public:
	// Default ctor; creates a "small" object representing a meta-text event
	// w/ a payload length of 0.  
	mtrk_event_t();
	// Ctor for callers who have pre-computed the exact size of the event and who
	// can also supply a midi status byte if applicible, ex, an mtrk_container_iterator_t.  
	mtrk_event_t(const unsigned char*, uint32_t, unsigned char=0);
	// Copy ctor
	mtrk_event_t(const mtrk_event_t&);
	// Copy assignment; overwrites a pre-existing lhs 'this' w/ rhs
	mtrk_event_t& operator=(const mtrk_event_t&);
	// Move ctor
	mtrk_event_t(mtrk_event_t&&) noexcept;
	// Move assignment
	mtrk_event_t& operator=(mtrk_event_t&&) noexcept;
	// Dtor
	~mtrk_event_t();

	// Iterators allowing access to the underlying unsigned char array
	//
	// begin(), dt_begin() both return iterators to the first byte of the 
	// dt field.  The redundant dt_ versions are here for users who want
	// to be more explicit.  dt_end() returns an iterator to the first
	// byte following the dt field, which is the first byte of the "event," 
	// and is the same as is returned by event_begin().  
	// payload_begin() returns an iterator to the first byte following
	// the delta_t field for midi events, and to the first byte following
	// the vlq length field for sysex and meta events.  
	mtrk_event_const_iterator_t begin() const;
	mtrk_event_iterator_t begin();
	mtrk_event_const_iterator_t dt_begin() const;
	mtrk_event_iterator_t dt_begin();
	mtrk_event_const_iterator_t dt_end() const;
	mtrk_event_iterator_t dt_end();
	mtrk_event_const_iterator_t event_begin() const;
	mtrk_event_iterator_t event_begin();
	mtrk_event_const_iterator_t payload_begin() const;
	mtrk_event_iterator_t payload_begin();
	mtrk_event_const_iterator_t end() const;
	mtrk_event_iterator_t end();	

	const unsigned char& operator[](uint32_t) const;
	unsigned char& operator[](uint32_t);
	unsigned char *data();
	const unsigned char *data() const;

	uint32_t data_size() const;  // Not including the delta-t
	uint32_t size() const;  // Includes delta-t
	// If is_small(), reports the size of the d_ array, which is the maximum
	// size of an event that the 'small' state can contain.  
	uint32_t capacity() const;

	// Getters
	unsigned char status_byte() const;
	smf_event_type type() const;
	uint32_t delta_time() const;
	bool set_delta_time(uint32_t);
	// For meta events w/ a text payload, copies the payload to a
	// std::string;  Returns an empty std::string otherwise.  
	std::string text_payload() const;
	// For channel events, gets the midi data, including.  For non-channel
	// events, .is_valid==false and the value of all other fields is 
	// undefined.  
	struct channel_event_data_t {
		uint8_t status_nybble {0x00u};  // most-significant nybble
		uint8_t ch {0x00u};
		uint8_t p1 {0x00u};
		uint8_t p2 {0x00u};
		bool is_valid {false};
		bool is_running_status {false};
	};
	channel_event_data_t midi_data() const;

	bool is_big() const;
	bool is_small() const;
	bool validate() const;
private:
	enum class offs {
		ptr = 0,
		size = 0 + sizeof(unsigned char*),
		cap = 0 + sizeof(unsigned char*) + sizeof(uint32_t),
		dt = 0 + sizeof(unsigned char*) + 2*sizeof(uint32_t),
		type = 0 + sizeof(unsigned char*) + 3*sizeof(uint32_t),
		// Not really an "offset":
		max_size_sbo = sizeof(unsigned char*) + 3*sizeof(uint32_t) + 2
	};

	//
	// Private data:  Overall object size is 24 bytes
	// std::array<unsigned char,22> d_;
	// unsigned char midi_status_;
	// unsigned char flags_;
	// 
	// midi_status_ is _always_ the status applicable to the object.  Thus, if
	// the object stores a midi event w/ no status byte (ie, the event is in
	// running-status), the value of midi_status_ is the corresponding running
	// status byte.  For midi messages w/ an event-local status byte, 
	// midi_status_ is a copy of this value.  For meta or sysex events, 
	// midi_status_ == 0x00u.  
	//
	// Case big:  Probably meta or sysex_f0/f7, but _could_ be midi (rs or 
	// non-rs); after all, there is no reason an event w/ size() < 22 bytes 
	// can not be put on the heap.  
	// Following the three "buffer parameters" ptr, size, capacity are two 
	// data members redundant w/ the data at p.  These are object-local 
	// "cached" versions of the remote data that prevent an indirection when
	// a user calls delta_time() or type().  The object must take care to keep
	// these in sync w/ the remote raw data (ex, on a call to 
	// set_delta_time()).  
	// 
	// d_ ~ { 
	//     unsigned char *p;      // cum sizeof()==8
	//     unit32_t size sz;      // cum sizeof()==12
	//     uint32_t capacity c;   // cum sizeof()==16
	//
	//     uint32_t delta_t.val;  // fixed size version of the vl quantity
	//     smf_event_type b21;    // 
	//     unsigned char b22;     // unused
	// };  // sizeof() => 22
	//
	//
	// Case small:  meta, sysex_f0/f7, midi (rs or non-rs), unknown.  
	// If a midi event and b1 is a valid status byte, b1 and midi_status must
	// match.  Otherwise, if a midi event and b1 is a data byte, midi_status
	// is the running-status value.  midi_status is always the value of the
	// midi status applic. to the present event, no matter what the state of
	// b1.  
	// d_ ~ {
	//     midi-vl-field delta_t;
	//     unsigned char b(delta_t.N+1);  // byte 1 following the vl dt field
	//     unsigned char b(delta_t.N+2);  // byte 2 following the vl dt field
	//     unsigned char b(delta_t.N+3);  // byte 3 following the vl dt field
	//     ...
	//     unsigned char b22;  // byte n-1 following the vl event start
	// };  // sizeof() => 22
	//
	std::array<unsigned char,22> d_ {0x00u};
	unsigned char midi_status_ {0x00u};  // always the applic. midi status
	unsigned char flags_ {0x80u};  // 0x00u=>big; NB:  defaults to "small"
	static_assert(static_cast<uint32_t>(offs::max_size_sbo)==sizeof(d_));
	static_assert(sizeof(d_)==22);
	static_assert(static_cast<uint32_t>(offs::max_size_sbo)==22);
	
	// Ptr to this->data_[0], w/o regard to this->is_small()
	const unsigned char *raw_data() const;
	// ptr to this->flags_
	const unsigned char *raw_flag() const;

	// Causes the container to "adopt" the data at p (writes p and the 
	// associated size, capacity into this->d_).  Note: data at p is _not_
	// copied into a new buffer.  Caller should not delete p after calling.  
	// The initial state of the container is completely overwritten w/o 
	// discretion; if is_big(), big_ptr() is _not_ freed.  Callers not wanting
	// to leak memory should first check is_big() and free big_ptr() as 
	// necessary.  This function does not rely in any way on the initial
	// state of the object; it is perfectly fine for the object to be 
	// initially in a completely invalid state.  
	// args:  ptr, size, cap, running status
	bool init_big(unsigned char *, uint32_t, uint32_t, unsigned char); 
	// If is_small(), make big with capacity as specified.  All data from the 
	// local d_ array is copied into the new remote buffer.  
	bool small2big(uint32_t);
	bool big_resize(uint32_t);

	// Zero all data members.  No attempt to query the state of the object
	// is made; if is_big() the memory will leak.  
	void clear_nofree();

	void set_flag_small();
	void set_flag_big();

	// Getters {big,small}_ptr() get a pointer to the first byte of
	// the underlying data array (the first byte of the dt field).  Neither
	// checks for the container size; the caller must be careful to call the
	// correct function.  These are the "unsafe" versions of .data().  
	unsigned char *big_ptr() const;  // getter
	unsigned char *small_ptr() const;  // getter
	unsigned char *set_big_ptr(unsigned char *p);  // setter
	// Getters to read the locally-cached size() field in the case of 
	// this->is_big().  Does not check if this->is_big(); must not be called
	// if this->is_small().  This is the unsafe version of .size().  
	uint32_t big_size() const;  // getter
	uint32_t set_big_size(uint32_t);  // setter
	uint32_t big_cap() const;  // getter
	uint32_t set_big_cap(uint32_t);  // setter
	uint32_t big_delta_t() const;  // shortcut to determining the ft if is_big()
	uint32_t set_big_cached_delta_t(uint32_t);
	smf_event_type big_smf_event_type() const;  // shortcut to determining the type if is_big()
	smf_event_type set_big_cached_smf_event_type(smf_event_type);

	friend std::string print(const mtrk_event_t&,
			mtrk_sbo_print_opts);
};



//
// Meta event classification
//
// I include the leading 0xFF byte so that it is possible to have "unknown"
// and "invalid" message types.  Some meta events have fixed-lengths; 
// functions classify_meta_event(), is_meta() etc do not verify these 
// values, and are not generic validation functions.  
//
enum class meta_event_t : uint16_t {
	seqn = 0xFF00u,
	text = 0xFF01u,
	copyright = 0xFF02u,
	trackname = 0xFF03u,
	instname = 0xFF04u,
	lyric = 0xFF05u,
	marker = 0xFF06u,
	cuepoint = 0xFF07u,
	chprefix = 0xFF20u,
	eot = 0xFF2Fu,
	tempo = 0xFF51u,
	smpteoffset = 0xFF54u,
	timesig = 0xFF58u,
	keysig = 0xFF59u,
	seqspecific = 0xFF7Fu,

	invalid = 0x0000u,
	unknown = 0xFFFFu
};
meta_event_t classify_meta_event_impl(const uint16_t&);

meta_event_t classify_meta_event(const mtrk_event_t&);
std::string print(const meta_event_t&);
bool is_meta(const mtrk_event_t&, const meta_event_t&);
bool is_meta(const mtrk_event_t&);
bool is_seqn(const mtrk_event_t&);
bool is_text(const mtrk_event_t&);
bool is_copyright(const mtrk_event_t&);
bool is_trackname(const mtrk_event_t&);
bool is_instname(const mtrk_event_t&);
bool is_lyric(const mtrk_event_t&);
bool is_marker(const mtrk_event_t&);
bool is_cuepoint(const mtrk_event_t&);
bool is_chprefix(const mtrk_event_t&);
bool is_eot(const mtrk_event_t&);
bool is_tempo(const mtrk_event_t&);
bool is_smpteoffset(const mtrk_event_t&);
bool is_timesig(const mtrk_event_t&);
bool is_keysig(const mtrk_event_t&);
// Returns true if the event is a meta_event_t w/a text payload,
// ex:  meta_event_t::text, meta_event_t::lyric, etc. 
bool meta_has_text(const mtrk_event_t&);
// Get the text of any meta event w/a text payload, ex:
// meta_event_t::text, ::copyright, ::trackname, instname::, ...
std::string meta_generic_gettext(const mtrk_event_t&);
// TODO:  std::vector<unsigned char> meta_generic_getpayload(...


//
// Channel event classification
// Note that for many events, the status byte, as well as p1 and p2 are
// needed to classify the event in a uswful way.  For example, where 
// s&0xF0u==0x90u, (nominally a note-on status byte), p2 has to be > 0 
// for the event to qualify as a note-on (otherwise it is a note-off).  
// Where s&0xF0u==0xB0u, p1 distinguishes a channel_voice from a channel_mode
// event.  
// For the purpose of creating an enum class to hold the channel_event types,
// an underlying type of uint8_t, or even uint16_t may be insufficient.  
// See also the enum class for meta event types where i included the leading 
// 0xFFu to allow for ::invalid and ::unrecognized events.  One important
// difference between here and the case of the meta events, however, is that
// the meta event enum class can be static_cast to a uint16_t to obtain the
// first two bytes of the meta event; this is not possible for channel events
// because of the nature of the equality conditions, which involve shifting
// and masking s, p1, p2, in different ways for each type.  
//
// In the future, this set of functions may be expanded to things like 
// is_bank_select(), is_pan(), is_foot_controller(), etc.  These require 
// reading p1.  
bool is_channel(const mtrk_event_t&);
bool is_channel_voice(const mtrk_event_t&);
bool is_channel_mode(const mtrk_event_t&);
// is_note_on(), is_note_off() evaluate both the status nybble as well as
// p2.  For a status nybble of 0x9nu, p2 must be > 0 for the  event to 
// qualify as a note-on event.  Events w/a status nybble of 0x9nu and p2==0 
// are considered note-off events.  
bool is_note_on(const mtrk_event_t&);
bool is_note_off(const mtrk_event_t&);
bool is_key_aftertouch(const mtrk_event_t&);  // 0xAnu
bool is_control_change(const mtrk_event_t&);
bool is_program_change(const mtrk_event_t&);
bool is_channel_aftertouch(const mtrk_event_t&);  // 0xDnu
bool is_pitch_bend(const mtrk_event_t&);




/*
//
// Meta events
//
class text_event_t {
public:
	// Creates an event w/ payload=="text event"
	text_event_t();
	explicit text_event_t(const std::string&);

	std::string text() const;
	uint32_t delta_time() const;
	uint32_t size() const;
	uint32_t data_size() const;

	// Setters return false upon failure (ex, if the caller attempts to
	// pass a value too large).  
	bool set_text(const std::string&);
	//bool set_delta_time(uint32_t);
private:
	mtrk_event_t d_;
};
*/


