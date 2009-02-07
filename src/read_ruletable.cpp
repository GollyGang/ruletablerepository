/*

This file is part of The Rule Table Repository.
Copyright (C) 2009 Tim Hutton.

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

*/

/*

Demonstrates how to read rule tables from file for your CA project.

To compile:
g++ read_ruletable.cpp ruletable.cpp -o read_ruletable
or in Microsoft Visual Studio, add files to an empty CLR project.

Contact: Tim Hutton <tim.hutton@gmail.com>

*/

#include "ruletable.h"

#include <string>
#include <iostream>
using namespace std;

int main()
{
   // parameters for use:
   const string input_filename = "wireworld.table";

   ruletable rules;

   cout << "Reading from " << input_filename << "...";
   string returned = rules.ReadFromFile(input_filename);
   if(returned.empty())
   {
      cout << "ok.\n";
      cout << "rule table has " << rules.GetNumStates() << " states.\n";
      cout << "rule table has the " << 
         ((rules.GetNeighborhood()==ruletable::vonNeumann)?"vonNeumann":"Moore") << " neighborhood.\n";
      cout << "rule table has symmetries: " << rules.GetSymmetry() << endl;
      cout << "example transition:\n";
      if(rules.GetNeighborhood()==ruletable::vonNeumann)
         cout << "0,0,0,0,0 -> " << rules.slowcalc(0,0,0,0,0,0,0,0,0) << endl;
      else
         cout << "0,0,0,0,0,0,0,0,0 -> " << rules.slowcalc(0,0,0,0,0,0,0,0,0) << endl;
   }
   else
      cout << "error!\n" << returned << endl;
}
