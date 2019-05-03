#pragma once
#include "midi_raw.h"
#include <cstdint>
#include <string>

// 
// The ..._unsafe_...() routines and friends deliberately return specific
// invalid values where the input does not satisfy the requirements of the
// routine.  That is, in some cases of invalid-input, they are able to 
// "poision" the return with info concerning the nature of the error.  If 
// they returned these maybe types instead of raw underlying, the caller 
// could implement an error-handling strategy at low cost.  
//
//
// Restricting the ctors of classes such that only valid object instances 
// can be constructed (ex by a factory or whoever) is the optimal way to
// design a class, but is not a solution to the problem being addressed 
// here.  Situations arise where construction must occur from runtime
// data and there needs to be a way to deal with the inability to
// instantiate an object.  
//
// Some factory function reads dynamic (run-time) input and attempts to 
// construct and return an object from the input.  The factory function is
// always (?) able to tell if the operation will be successfull; the factory
// is a friend of the class and knows its invariants.  At some point the
// factory decides the input does not satisfy the constraints of the class
// for some well-determined reason.  There are two situations:
// 1)  The sizeof() the class that the factory is attempting to construct is
//     such that all possible errors can be encoded as special values in the
//     space occupied by the data members of the class.  It is possible to 
//     examine the object and see that it is invalid.  
// 2)  The sizeof() the class that the factory is attempting to construct is
//     such that there is no space in the class to store error values.  
//     Objects that do not satisfy the invariants of the class are 
//     indistinguishible from objects that do.  
//
// Case (1):
// A free (or class-member) is_valid(class_t) function can be written to 
// extract the error information packed into the class by the factory.  If
// using the convention that all objects are valid (class_t is never a 
// maybe type), this is gross.  Why have a .is_valid() for a guaranteed-valid
// object?  
// Reject.  
//
// Alternatively, all such factories can return an instance of:
// maybe_result_t<class_t,class_t_err_t>
// A free is_valid(class_t,class_t_err_t), where the arg class_t is just 
// acting as an uninstantiated tag, is written for each class_t for which 
// such a maybe_result_t can be constructed.  For case 1 maybe class_t_err_t
// can be void (?).  
//



struct delta_time {
	using underlying_type = uint32_t;
	using underlying_error_type = uint8_t;
	underlying_type val_;
};
bool is_valid(unsigned char,delta_time);
std::string explain_err(unsigned char,delta_time);
struct midi_status_byte {
	using underlying_type = unsigned char;
};
bool is_valid(unsigned char,midi_status_byte);
std::string explain_err(unsigned char,midi_status_byte);
struct midi_data_byte {
	using underlying_type = unsigned char;
};
bool is_valid(unsigned char,midi_data_byte);
std::string explain_err(unsigned char,midi_data_byte);

template<typename T, typename E=void>
class maybe_result_t {
public:
	using underlying_type = typename T::underlying_type;
	operator bool() const {
		return is_valid(T,E);
	};
	bool set(underlying_type v) {
		this->val_ = v;
		return is_valid(this->val_,T);
	};
	underlying_type get() const {
		return this->val_;
	};

	std::string explain() const {
		return explain_err(this->val_,T);
	};

	union u_t_err {
		T val_;
		E err_;
	};
	struct taggedu_t {
		union u_t_err u_;
		bool is_valid_;
	};
	taggedu_t d_;
private:
};


maybe_result_t<midi_status_byte>
	stream_get_midi_status_byte_dtstart(const unsigned char*, unsigned char=0x00u);




