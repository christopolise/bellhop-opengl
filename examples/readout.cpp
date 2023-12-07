/*
bellhopcxx / bellhopcuda - C++/CUDA port of BELLHOP(3D) underwater acoustics simulator
Copyright (C) 2021-2023 The Regents of the University of California
Marine Physical Lab at Scripps Oceanography, c/o Jules Jaffe, jjaffe@ucsd.edu
Based on BELLHOP / BELLHOP3D, which is Copyright (C) 1983-2022 Michael B. Porter

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <iostream>

// This define must be set before including the header if you're using the DLL
// version on Windows, and it must NOT be set if you're using the static library
// version on Windows. If you're not on Windows, it doesn't matter either way.
#define BHC_DLL_IMPORT 1
#include <bhc/bhc.hpp>

void OutputCallback(const char *message)
{
    std::cout << "Out: " << message << std::endl << std::flush;
}

void PrtCallback(const char *message) { std::cout << message << std::flush; }

static bhc::bhcInit init;
static std::string FileRootIn, FileRootOut;

template<bool O3D, bool R3D> int mainmain()
{
    bhc::bhcParams<O3D> params;
    bhc::bhcOutputs<O3D, R3D> outputs;
    init.FileRoot       = FileRootIn.c_str();
    init.numThreads     = 1;
    init.prtCallback    = PrtCallback;
    init.outputCallback = OutputCallback;
    if(!bhc::setup<O3D, R3D>(init, params, outputs)) return 1;
    bhc::echo<O3D>(params);
    if(!bhc::readout<O3D>(params, outputs, nullptr)) return 1;
    if(!bhc::writeout<O3D>(params, outputs, FileRootOut.c_str())) return 1;
    bhc::finalize<O3D, R3D>(params, outputs);
    return 0;
}

void showhelp(const char *argv0)
{
    std::cout
        << "bellhopcxx - C++ port of BELLHOP underwater acoustics simulator\n"
           "\n"
           "Copyright (C) 2021-2023 The Regents of the University of California\n"
           "Marine Physical Lab at Scripps Oceanography, c/o Jules Jaffe, "
           "jjaffe@ucsd.edu\n"
           "Based on BELLHOP, which is Copyright (C) 1983-2022 Michael B. Porter\n"
           "GPL3 licensed, no warranty, see LICENSE or https://www.gnu.org/licenses/\n"
           "\n"
           "Usage: "
        << argv0
        << " [options] FileRootIn FileRootOut\n"
           "FileRootIn is the absolute or relative path to the input environment \n"
           "file, minus the .env file extension, e.g. test/in/MunkB_ray_rot .\n"
           "This environment file must have already been run and the data files\n"
           "produced by it must exist, e.g. test/in/MunkB_ray_rot.ray .\n"
           "These data files will be read into memory, and then written out to\n"
           "the corresponding output data files at FileRootOut.\n"
           "All command-line options may be specified with one or two dashes, e.g.\n"
           "-3 or --3 do the same thing. Furthermore, all command-line options have\n"
           "multiple synonyms which do the same thing.\n"
           "\n"
           "-?, -h, -help: Shows this help message\n"
           "-2, -2D: Does a 2D run. The environment file must also be 2D\n"
           "-3, -3D: Does a 3D run. The environment file must also be 3D\n"
           "-4, -Nx2D, -2D3D, -2.5D: Does a Nx2D run. The environment file must also be "
           "Nx2D\n";
}

int main(int argc, char **argv)
{
    int dimmode = 0;
    for(int32_t i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if(argv[i][0] == '-') {
            if(s.length() >= 2 && argv[i][1] == '-') { // two dashes
                s = s.substr(1);
            }
            if(s == "-2" || s == "-2D") {
                dimmode = 2;
            } else if(s == "-Nx2D" || s == "-2D3D" || s == "-2.5D" || s == "-4") {
                dimmode = 4;
            } else if(s == "-3" || s == "-3D") {
                dimmode = 3;
            } else if(s == "-?" || s == "-h" || s == "-help") {
                showhelp(argv[0]);
                return 0;
            } else {
                std::cout << "Unknown command-line option \"-" << s << "\", try "
                          << argv[0] << " --help\n";
                return 1;
            }
        } else if(FileRootIn.empty()) {
            FileRootIn = s;
        } else if(FileRootOut.empty()) {
            FileRootOut = s;
        } else {
            std::cout << "Error, received another command-line argument \"" << s
                      << "\", already have FileRootIn = \"" << FileRootIn
                      << "\" and FileRootOut = \"" << FileRootOut << "\"\n";
            return 1;
        }
    }
    if(FileRootIn.empty()) {
        std::cout << "Must provide FileRootIn as command-line parameter, try " << argv[0]
                  << " --help\n";
        return 1;
    }
    if(FileRootOut.empty()) {
        std::cout << "Must provide FileRootOut as command-line parameter, try " << argv[0]
                  << " --help\n";
        return 1;
    }
    if(dimmode < 2 || dimmode > 4) {
        std::cout << "No dimensionality specified (--2D, --Nx2D, --3D), assuming 2D\n";
        dimmode = 2;
    }
    if(dimmode == 2) { return mainmain<false, false>(); }
    if(dimmode == 3) { return mainmain<true, true>(); }
    if(dimmode == 4) { return mainmain<true, false>(); }
    return 1;
}
