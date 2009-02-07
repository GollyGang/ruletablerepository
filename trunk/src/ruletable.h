                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2008 Andrew Trevorrow and Tomas Rokicki.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Web site:  http://sourceforge.net/projects/golly
 Authors:   rokicki@gmail.com  andrew@trevorrow.com

                        / ***/

// Adapted from Golly's ruletable_algo.h

#include <string>
#include <vector>

typedef int state; 

/// A class that reads CA rule tables from file
class ruletable
{

public:

   /// return string is error description if failed, else empty
   std::string ReadFromFile(const std::string& filename);

   /// how many states does this ruleset define?
   int GetNumStates() { return n_states; }

   /// this is the update function: from these inputs, what is the output state
   /// (for von Neumann neighborhoods, pass any value for the unused inputs)
   state slowcalc(state nw, state n, state ne, state w, state c,
                state e, state sw, state s, state se);

   enum TNeighborhood { vonNeumann, Moore };
   enum TSymmetry { none, rotate4, rotate8, reflect, rotate4reflect, rotate8reflect };

   /// what is the neighborhood of the loaded rule?
   TNeighborhood GetNeighborhood() { return neighborhood; }

   /// what is the symmetry set of the loaded rule?
   TSymmetry GetSymmetry() { return symmetries; }

protected:

   unsigned int n_states;
   TNeighborhood neighborhood; // (hopefully will support more in future)
   TSymmetry symmetries;

   // we use a lookup table to match inputs to outputs:
   typedef unsigned long long int TBits; 
   // (we can use unsigned int if we hit portability issues (not much slower))

   std::vector< std::vector< std::vector<TBits> > > lut; 
   // TBits lut[neighbourhood_size][n_states][n_compressed_rules];

   unsigned int n_compressed_rules;
   std::vector<state> output; // state output[n_rules];

};

