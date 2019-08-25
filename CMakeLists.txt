cmake_minimum_required(VERSION 3.1...3.15)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project("Janice MIDI"
	VERSION 1.0 
	#DESCRIPTION "Janice MIDI"
	#HOMEPAGE_URL "https://github.com/nh2oh/jmidi"
	LANGUAGES CXX
)
message("CMAKE_PROJECT_NAME == ${CMAKE_PROJECT_NAME}")

add_library(
	jmidi
	src/aux_types.cpp  src/generic_chunk_low_level.cpp  src/generic_iterator.cpp  src/midi_delta_time.cpp  src/midi_examples.cpp  src/midi_raw_test_parts.cpp  src/midi_status_byte.cpp  src/midi_time.cpp  src/midi_vlq.cpp  src/mthd_t.cpp  src/mtrk_event_methods.cpp  src/mtrk_event_t.cpp  src/mtrk_event_t_internal.cpp  src/mtrk_integrators.cpp  src/mtrk_t.cpp  src/print_hexascii.cpp  src/smf_t.cpp  src/util.cpp

	include/aux_types.h  include/generic_chunk_low_level.h  include/generic_iterator.h  include/midi_data.h  include/midi_delta_time.h  include/midi_examples.h  include/midi_raw_test_parts.h  include/midi_status_byte.h  include/midi_time.h  include/midi_vlq.h  include/mthd_t.h  include/mtrk_event_methods.h  include/mtrk_event_t.h  include/mtrk_event_t_internal.h  include/mtrk_integrators.h  include/mtrk_t.h  include/print_hexascii.h  include/smf_t.h  include/util.h
)
target_include_directories(jmidi PUBLIC include)
target_compile_features(jmidi PUBLIC cxx_std_17)

#find_package(Threads)

add_executable(tests
	tests/delta_time_test_data.cpp  tests/delta_time_test_data.h  
	tests/make_mtrk_event_tests.cpp  tests/midi_chunk_low_level_tests.cpp  
	tests/midi_dt_tests.cpp  tests/midi_raw_test_data.cpp  tests/midi_raw_test_data.h
	tests/midi_sb_classification_funcs.cpp  tests/midi_tempo_and_timediv_tests.cpp  
	tests/midi_testdata_status_bytes.cpp  tests/midi_testdata_status_bytes.h  
	tests/midi_vlq_tests.cpp  tests/mthd_test_data.cpp  tests/mthd_test_data.h  
	tests/mthd_t_test.cpp  tests/mtrk_duration_tests.cpp  
	tests/mtrk_event_class_method_tests.cpp  tests/mtrk_event_ctor_tests.cpp  
	tests/mtrk_event_internal.cpp  tests/mtrk_event_interrogation_ch_events.cpp  
	tests/mtrk_event_sysex_factory_funcs.cpp  
	tests/mtrk_event_t_meta_factory_funcs_tests.cpp  tests/mtrk_event_t_tests.cpp 
	tests/mtrk_special_member_function_tests.cpp  tests/mtrk_test_data.cpp  
	tests/mtrk_test_data.h  tests/mtrk_t_split_merge_test.cpp  tests/mtrk_t_test.cpp
	tests/smf_chrono_iterator_test.cpp  tests/sysex_factory_test_data.cpp  
	tests/sysex_factory_test_data.h
)
target_link_libraries(tests PUBLIC jmidi)
find_package(GTest)
# "module mode" (default):  CMake searches for a file called Find<PackageName>.cmake. 
#     The file is first searched in the CMAKE_MODULE_PATH,  then among the "Find Modules"
#     provided by the CMake installation...
target_link_libraries(tests PUBLIC GTest::GTest GTest::Main)
#target_link_libraries(tests PUBLIC gtest gmock gtest_main)
#target_link_libraries(tests PUBLIC ${CMAKE_THREAD_LIBS_INIT})


add_executable(event_sizes_benchmark
	examples/event_sizes_benchmark/event_sizes_benchmark.cpp
	examples/event_sizes_benchmark/event_sizes_benchmark.h
)
target_compile_features(event_sizes_benchmark PUBLIC cxx_std_17)
#find_package(Threads)
#target_link_libraries(event_sizes_benchmark PUBLIC ${CMAKE_THREAD_LIBS_INIT}) 
target_link_libraries(event_sizes_benchmark PUBLIC jmidi)

add_executable(vlq_benchmark
	examples/vlq_benchmark/vlq_benchmark.cpp
	examples/vlq_benchmark/vlq_benchmark.h
)
target_compile_features(vlq_benchmark PUBLIC cxx_std_17)
target_link_libraries(vlq_benchmark PUBLIC jmidi)


add_executable(smfprint
	examples/smfprint/smfprint.cpp
)
target_compile_features(smfprint PUBLIC cxx_std_17)
target_link_libraries(smfprint PUBLIC jmidi)



#
# Cmake Notes:
#
# Never:
# add_compile_options, include_directories, link_directories, link_libraries
# CMAKE_CXX_FLAGS

# find_library() as an alternative to find_package()


# Write a Config.cmake file: This is what a library author should do to support clients.



