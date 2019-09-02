#include <cstdint>
#include <utility>
#include <string>
#include <filesystem>


class dfa {
public:
	enum class state : std::int8_t {
		event_start,
		ch_event_in_rs,
		ch_event_not_rs,
		meta_event,
		sysex_event,
		finished,
		invalid
	};
	state init(const unsigned char *, const unsigned char *);
	std::pair<dfa::state,const unsigned char*> get_next();
	bool finished();
private:
	const unsigned char *p_ {nullptr};
	const unsigned char *end_ {nullptr};
	state st_ {state::event_start};
	unsigned char rs_ {0};
};

std::string print(dfa::state);

int dfa_parse_midis(const std::filesystem::path&);



class dfa2 {
public:
	enum class state : std::int8_t {
		delta_time,
		status_byte,
		meta_type,
		vlq_payl_length,
		payload,
		at_end_success,
		invalid
	};
	dfa2() = default;
	explicit dfa2(unsigned char rs) : rs_(rs) {};

	bool operator()(unsigned char);
	dfa2::state get_state() const;
	unsigned char get_rs() const;
private:
	uint32_t val_ {0};
	uint32_t max_iter_remain_ {4};
	dfa2::state st_ {dfa2::state::delta_time};
	unsigned char rs_ {0};

	friend std::string print(const dfa2&);
};
std::string print(dfa2::state);
std::string print(const dfa2&);

int dfa2_parse_midis(const std::filesystem::path&);

