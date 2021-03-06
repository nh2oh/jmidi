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
	include/aux_types.h  src/aux_types.cpp
	include/generic_chunk_low_level.h  src/generic_chunk_low_level.cpp
	include/generic_iterator.h  src/generic_iterator.cpp
	include/make_mtrk_event.h  src/make_mtrk_event.cpp
	include/midi_data.h
	include/midi_delta_time.h  src/midi_delta_time.cpp
	include/midi_raw_test_parts.h  src/midi_raw_test_parts.cpp
	include/midi_status_byte.h  src/midi_status_byte.cpp
	include/midi_time.h  src/midi_time.cpp
	include/midi_vlq.h  src/midi_vlq.cpp
	include/mthd_t.h  src/mthd_t.cpp
	include/mtrk_event_methods.h  src/mtrk_event_methods.cpp
	include/mtrk_event_t.h  src/mtrk_event_t.cpp
	include/mtrk_integrators.h  src/mtrk_integrators.cpp
	include/mtrk_t.h  src/mtrk_t.cpp
	include/print_hexascii.h  src/print_hexascii.cpp
	include/small_bytevec_t.h  src/small_bytevec_t.cpp
	include/smf_t.h  src/smf_t.cpp
	include/util.h  src/util.cpp
)
target_include_directories(jmidi PUBLIC include)
target_compile_features(jmidi PUBLIC cxx_std_17)

#find_package(Threads)

add_executable(tests
	tests/delta_time_test_data.cpp  tests/delta_time_test_data.h  
	tests/make_mtrk_event3.cpp  tests/midi_chunk_low_level_tests.cpp  
	tests/midi_dt_tests.cpp  tests/midi_raw_test_data.cpp  tests/midi_raw_test_data.h
	tests/midi_sb_classification_funcs.cpp  tests/midi_tempo_and_timediv_tests.cpp  
	tests/midi_testdata_status_bytes.cpp  tests/midi_testdata_status_bytes.h  
	tests/midi_vlq_tests.cpp  tests/mthd_test_data.cpp  tests/mthd_test_data.h  
	tests/mthd_t_test.cpp  tests/mtrk_duration_tests.cpp  
	tests/mtrk_event_class_method_tests.cpp  tests/mtrk_event_ctor_tests.cpp  
	tests/small_bytevec_tests.cpp  tests/mtrk_event_interrogation_ch_events.cpp  
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
find_package(Threads)
target_link_libraries(event_sizes_benchmark PUBLIC ${CMAKE_THREAD_LIBS_INIT}) 
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



