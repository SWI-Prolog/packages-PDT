cmake_minimum_required(VERSION 2.8.12)
project(swipl-PDT)

include("../cmake/PrologPackage.cmake")

swipl_plugin(
    pdt_console
    C_SOURCES pdt_console.c
    PL_LIBS pdt_console.pl)
