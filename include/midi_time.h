#pragma once
#include <cstdint>
#include <string>
#include <array>

namespace jmid {

//
// Class jmid::time_division_t
//
// Represents a 2-byte time-division field in an MThd chunk.  It is
// impossible for a jmid::time_division_t to hold a value that is invalid for a
// time division field.  Ctors silently convert invalid inputs into valid
// values.  
//
// The "support" class smpte_t represents the pair of SMPTE fields; it
// does not enforce any invariants and is only used for as a convenient
// return type for the jmid::time_division_t::get_smpte() getter and for the
// SMPTE ctor jmid::time_division_t::jmid::time_division_t(smpte_t).  
// 
struct smpte_t {
	std::int32_t time_code;
	std::int32_t subframes;
};
class time_division_t {
public:
	enum class type : uint8_t {
		ticks_per_quarter,
		smpte
	};

	// 120 tiks-per-quarter; type() == ticks_per_quarter
	time_division_t()=default;
	// Construct a ticks-per-quarter-valued time-division object
	// The input is clamped to [1,32767] (32767==0x7FFF).  
	explicit time_division_t(std::int32_t);
	// This is declared deleted b/c is is ambiguous; does the user mean for
	// the uint16_t argument to be interpreted as a number of tpq, or as a
	// "raw value," as by make_time_division_from_raw(uint16_t)?
	explicit time_division_t(std::uint16_t)=delete;
	// Construct a SMPTE-valued time-division object
	// Allowed values for arg 1 (SMPTE-time-code) are -24, -25, -29, -30.  
	// If an invalid value is passed in, -24 is substituted.  The value for
	// arg 2 (subdivisions-per-frame) is clamped to [1,255].  
	explicit time_division_t(std::int32_t, std::int32_t);
	explicit time_division_t(smpte_t);
	
	// Getters
	// If called on an object of the wrong type, the value returned 
	// will be valid as the requested type of quantity, but will not 
	// correspond to the value held in the object.  A jmid::time_division_t 
	// /never/ holds or returns invalid data.  
	// See also the non-member T get_*(...) family of functions
	jmid::time_division_t::type get_type() const;
	std::uint16_t get_raw_value() const;
	smpte_t get_smpte() const;
	std::int32_t get_tpq() const;

	bool operator==(const jmid::time_division_t&) const;
	bool operator!=(const jmid::time_division_t&) const;
private:
	// SMPTE => Society of Motion Picture and Television Engineers 
	// 16-bit:  [[1] frames-per-second] [resolution-within-frame]
	// Number of subframes per frame ("units per frame," "ticks per frame")
	// Legal values (std p.133): "4 (MIDI time code resolution),
	// 8, 10, 80 (bit resolution), or 100"
	std::array<unsigned char,2> d_ {0x00u,0x78u};  // 120 ticks-per-quarter
};

//
// maybe_jmid::time_division_t make_*() "Factory" functions
//
// For invalid input, .is_valid == false, however, the jmid::time_division_t 
// object in .value will always contain a valid value (a consequence of the
// hard invariants enfoeced by the time_divison_t class).  Where is_valid
// == false, the object in .value obviously does not correspond to the 
// values passed to the make_*() function.  
//
struct maybe_time_division_t {
	jmid::time_division_t value;
	bool is_valid {false};
};
// maybe_jmid::time_division_t
// make_time_division_smpte(int32_t SMPTE-time-code, int32_t subdivs);
// maybe_jmid::time_division_t
// make_jmid::time_division_tpq(int32_t ticks-per-quarter-note);
maybe_time_division_t make_time_division_smpte(smpte_t);
maybe_time_division_t make_time_division_smpte(std::int32_t,std::int32_t);
maybe_time_division_t make_time_division_tpq(std::int32_t);
// maybe_jmid::time_division_t
// make_time_division_from_raw(int16_t raw_val);
// raw_val is the value of the d_ array "deserialized" as a BE-encoded 
// quantity; same as returned by the jmid::time_division_t::raw_value() getter.  
maybe_time_division_t make_time_division_from_raw(std::uint16_t);
bool is_smpte(time_division_t);
bool is_tpq(time_division_t);
bool is_valid_time_division_raw_value(std::uint16_t);
//
// T get_*(jmid::time_division_t tdiv) functions
//
// These are provided alternatives to the jmid::time_division_t-member getter  
// methods T jmid::time_division_t::get_*().  The member getters always return
// a valid value, even where a getter is called on an object of incorrect 
// type (ex, if jmid::time_division_t::get_smpte() is called on a jmid::time_division_t
// with type()==jmid::time_division_t::type::ticpq_per_quarter).  These non-member 
// "getters" return a caller-provided default value if the jmid::time_division_t
// has type() not corresponding to the getter.  
// 
smpte_t get_smpte(jmid::time_division_t, smpte_t={0,0});
std::int32_t get_tpq(jmid::time_division_t, std::int32_t=0);


// double ticks2sec(const int32_t& ticks, const jmid::time_division_t& tdiv, 
//					int32_t tempo=500000);
// For a smpte tdiv, the tempo argument is ignored, and 
// seconds = ticks/(smpte.time_code*smpte_subframes)
// For a tpq tdiv, 
// seconds = ticks*(tempo/tpq)
double ticks2sec(std::int32_t, const time_division_t&, std::int32_t=500000);
// int32_t sec2ticks(const double& sec, const jmid::time_division_t& tdiv,
//						int32_t tempo=500000);
// For a smpte tdiv, the tempo argument is ignored, and 
// ticks = sec/(smpte.time_code*smpte_subframes)
// For a tpq tdiv, 
// ticks = sec*(tpq/tempo)
std::int32_t sec2ticks(const double&, const time_division_t&, std::int32_t=500000);


//
// Note values
//
// The struct note_value_t represents the duration of a note relative to
// a whole note.  A note note_value_t nt with exp=nt.exp and ndot=nt.ndot
// has duration == ((1/2)^exp)*((1+2^ndot)/(2^ndot)) number of whole 
// notes.  
//
// int32_t note2ticks(int32_t pow2, int32_t ndots, 
//						const jmid::time_division_t& tdiv);
// Returns 0 for a smpte-type jmid::time_division_t and/or if ndots == 0.  
//
// pow2 == -1 => double-whole note
// pow2 ==  0 => whole note
// pow2 ==  1 => half note
// pow2 ==  2 => quarter note
// pow2 ==  3 => eighth note
// pow2 ==  4 => sixteenth note
//
std::int32_t note2ticks(std::int32_t, std::int32_t, const time_division_t&);
struct note_value_t {
	std::int32_t exp;
	std::int32_t ndot;
};
note_value_t ticks2note(std::int32_t, const time_division_t&);
std::string print(const note_value_t&);
// TODO:  Under what circumstances are these calcs inexact?  What 
// kind of error guarantees can i make?



}  // namespace jmid

