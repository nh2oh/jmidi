#pragma once
#include "mtrk_event_t.h"
#include "mtrk_event_methods.h"
#include <string>
#include <cstdint>



// TODO:  Apply S. Parent's concept that inh should be an
// implementation detail.  
//
// TODO:  I want to be able to bundle these @ low cost.  Ex,
// i may want my lyric integrator to also associate a cumtk
// or a ms value w/ the last-encountered lyric event.  When 
// expanded to smf_t's & iterating over a group of tracks i 
// may also want to assoctate a trackn w/ event data held by the
// integrator.  
//
// NB:  For a user looping through an mtrk_event_t sequence and
// updating some vector of integrators, in the general case, += 
// for each integrator has to be called for each mtrk event, since
// more than one integrator may apply to any given event.  Ex,
// a lyric event has to be processed by the lyric integrator _and_
// by the delta_time integrator.  The integrator vector could be 
// wrapped into it's own type w/a custom += to be more effecient.  
struct integrator_t {
	virtual integrator_t& operator+=(const mtrk_event_t& ev)=0;
	virtual std::string print()=0;
};
struct tk_integrator_t : integrator_t {
	tk_integrator_t& operator+=(const mtrk_event_t& ev) override {
		this->val_ += ev.delta_time();
		return *this;
	};
	std::string print() override {
		return ("tk_integrator_t.val_=="+std::to_string(this->val_));
	};
	uint32_t val_ {0};
};
struct lyric_integrator_t : integrator_t {
	lyric_integrator_t& operator+=(const mtrk_event_t& ev) override {
		if (is_lyric(ev)) {
			this->val_ = meta_generic_gettext(ev);  // ev.text_payload();
			return *this;
		}
	};
	std::string print() override {
		return ("lyric_integrator_t.val_=="+this->val_);
	};
	std::string val_ {};
};
// Idea:  Member tempo-track; instantiate w/a tempo track 
struct time_integrator_t : integrator_t {
	time_integrator_t& operator+=(const mtrk_event_t& ev) override {
		this->val_ += ev.delta_time()*(this->tempo_/this->tpq_)/1000;
		this->tempo_ = get_tempo(ev,this->tempo_);
		return *this;
	};
	std::string print() override {
		return ("time_integrator_t.val_=="+std::to_string(this->val_)
			+"; tempo=="+std::to_string(this->tempo_));
	};
	double val_ {0};
	uint32_t tempo_ {500000};
	uint32_t tpq_ {96};  // ticks per q nt from MThd
};


