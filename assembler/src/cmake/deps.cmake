# -*- cmake -*-

find_package(OpenMP)
find_package(ZLIB REQUIRED)
find_package(Readline QUIET)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses QUIET)

# Use included boost unless explicitly specified
if (NOT SPADES_BOOST_ROOT)
  set(BOOST_ROOT "${EXT_DIR}/include")
else()
  set(BOOST_ROOT SPADES_BOOST_ROOT)
endif()
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED)
