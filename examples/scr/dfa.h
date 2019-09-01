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

