# Copyright (c) 2024, Jan Kuliga.

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sid-lab-automation)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
