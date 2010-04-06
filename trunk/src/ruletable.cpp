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

// Adapted from Golly's ruletable_algo.cpp

#include "ruletable.h"

// for case-insensitive string comparison
#include <string.h>
#ifndef WIN32
   #define stricmp strcasecmp
   #define strnicmp strncasecmp
#endif

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>
#include <string>
#include <fstream>
using namespace std;

#include <iostream> // for DEBUG

// helper function
bool starts_with(const string& line,const string& keyword)
{
   return strnicmp(line.c_str(),keyword.c_str(),keyword.length())==0;
}

// helper function
vector<string> tokenize(const string& str,const string& delimiters)
{
   vector<string> tokens;

   // skip delimiters at beginning.
   string::size_type lastPos = str.find_first_not_of(delimiters, 0);

   // find first "non-delimiter".
   string::size_type pos = str.find_first_of(delimiters, lastPos);

   while (string::npos != pos || string::npos != lastPos)
   {
      // found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));

      // skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(delimiters, pos);

      // find next "non-delimiter"
      pos = str.find_first_of(delimiters, lastPos);
   }

   return tokens;
}

string ruletable::ReadFromFile(const string& filename)
{
   const string comment_keyword = "#";
   const string symmetries_keyword = "symmetries:";
   const string neighborhood_keyword = "neighborhood:";
   const string neighborhood_value_keywords[2]={"vonNeumann","Moore"};
   const string n_states_keyword = "n_states:";
   const string variable_keyword = "var ";
   const string symmetry_keywords[6] = {"none","rotate4","rotate8","reflect",
                                        "rotate4reflect","rotate8reflect"};

   string line;
   int lineno = 0;

   // look for rule.table in user's rules dir then in Golly's rules dir
   ifstream in(filename.c_str());
   if(in.bad())
      return "Failed to open file: "+filename;

   this->symmetries = rotate4; // default
   this->n_states = 8;  // default

   map< string, vector<state> > variables;
   vector< pair<vector< vector<state> >, state > > transition_table;

   unsigned int n_inputs=0;

   // these line must have been read before the rest of the file
   bool n_states_parsed=false,neighborhood_parsed=false,symmetries_parsed=false;

   for (;;) 
   {
      if(in.eof()) break;
      getline(in,line);

      lineno++ ;
      int allws = 1 ;
      for (unsigned int i=0; i<line.size(); i++)
         if (line[i] > ' ') {
            allws = 0 ;
            break ;
         }
      if(starts_with(line,comment_keyword) || allws)
         continue; // comment line
      else if(starts_with(line,n_states_keyword))
      {
         // parse the rest of the line
         if(sscanf(line.c_str()+n_states_keyword.length(),"%d",&this->n_states)!=1)
         {
            ostringstream oss;
            oss << "Error reading " << filename << " on line " << lineno << ": " << line;
            return oss.str();
         }
         n_states_parsed = true;
      }
      else if(starts_with(line,symmetries_keyword))
      {
         string remaining(line.begin()+symmetries_keyword.length(),line.end());
         bool found_symmetry=false;
         for(int iS=0;iS<6;iS++)
            if(starts_with(remaining,symmetry_keywords[iS]))
            {
               this->symmetries = (TSymmetry)iS;
               found_symmetry = true;
            }
         if(!found_symmetry)
         {
            ostringstream oss;
            oss << "Error reading " << filename << " on line " << lineno << ": " << line;
            return oss.str();
         }
         symmetries_parsed = true;
      }
      else if(starts_with(line,neighborhood_keyword))
      {
         // parse the rest of the line
         string remaining(line.begin()+neighborhood_keyword.length(),line.end());
         bool found_neighborhood=false;
         for(int iN=0;iN<2;iN++)
            if(starts_with(remaining,neighborhood_value_keywords[iN]))
            {
               this->neighborhood = (TNeighborhood)iN;
               found_neighborhood = true;
            }
         if(!found_neighborhood)
         {
            ostringstream oss;
            oss << "Error reading " << filename << " on line " << lineno << ": " << line;
            return oss.str();
         }
         switch(this->neighborhood) {
            default:
            case vonNeumann: n_inputs=5; break;
            case Moore: n_inputs=9; break;
         }
         neighborhood_parsed=true;
      }
      else if(starts_with(line,variable_keyword))
      {
         if(!n_states_parsed || !neighborhood_parsed || !symmetries_parsed)
         {
            ostringstream oss;
            oss << "Error reading " << filename << ": one or more of n_states, neighborhood or symmetries missing\nbefore first variable";
            return oss.str();
         }
         // snip off any trailing comment
         if(line.find('#')!=string::npos)
            line.assign(line.begin(),line.begin()+line.find('#'));
         // parse the rest of the line for the variable
         vector<string> tokens = tokenize(line,"= {,}");
         string variable_name = tokens[1];
         vector<state> states;
         if(tokens.size()<3)
         {
            ostringstream oss;
            oss << "Error reading " << filename << " on line " << lineno << ": " << line;
            return oss.str();
         }  
         for(unsigned int i=2;i<tokens.size();i++)
         {
            if(variables.find(tokens[i])!=variables.end())
            {
               // variables permitted inside later variables
               states.insert(states.end(),variables[tokens[i]].begin(),variables[tokens[i]].end());
            }
            else
            {
                unsigned int s;
                if(sscanf(tokens[i].c_str(),"%d",&s)!=1)
                {
                   ostringstream oss;
                   oss << "Error reading " << filename << " on line " << lineno << ": " << line;
                   return oss.str();
                }
                if(s<0 || s>=this->n_states)
                {
                   ostringstream oss;
                   oss << "Error reading " << filename << " on line " << lineno << ": " << line << " - state value out of range";
                   return oss.str();
                }
                states.push_back((state)s);
             }
         }
         variables[variable_name] = states;
         
      }
      else
      {
         // must be a transitions line
         if(!n_states_parsed || !neighborhood_parsed || !symmetries_parsed)
         {
            ostringstream oss;
            oss << "Error reading " << filename << ": one or more of n_states, neighborhood or symmetries missing\nbefore first transition";
            return oss.str();
         }
         if(this->n_states<=10 && variables.empty())
         {
            vector< vector<state> > inputs;
            state output;
            // if there are single-digit states and no variables then use compressed form
            // e.g. 012345 for 0,1,2,3,4 -> 5
            if(line.length() < n_inputs+1) // we allow for comments after the rule
            {
               ostringstream oss;
               oss << "Error reading " << filename << " on line " << lineno << ": " << line << " - too few entries";
               return oss.str();
            }
            for(unsigned int i=0;i<n_inputs;i++)
            {
               char c = line[i];
               if(c<'0' || c>'9')
               {
                  ostringstream oss;
                  oss << "Error reading " << filename << " on line " << lineno << ": " << line;
                  return oss.str();
               }
               inputs.push_back(vector<state>(1,c-'0'));
            }
            unsigned char c = line[n_inputs];
            if(c<'0' || c>'9')
            {
               ostringstream oss;
               oss << "Error reading " << filename << " on line " << lineno << ": " << line;
               return oss.str();
            }
            output = c-'0';
            transition_table.push_back(make_pair(inputs,output));
         }
         else 
         {  
            vector<string> tokens = tokenize(line,", #\t");
            if(tokens.size() < n_inputs+1)
            {
               ostringstream oss;
               oss << "Error reading " << filename << " on line " << lineno << ": " << line << " - too few entries";
               return oss.str();
            }
            // first pass: which variables appear more than once? these are "bound" (must take the same value each time they appear in this transition)
            vector<string> bound_variables;
            for(map< string, vector<state> >::const_iterator var_it=variables.begin();var_it!=variables.end();var_it++)
               if(count(tokens.begin(),tokens.begin()+n_inputs+1,var_it->first)>1)
                  bound_variables.push_back(var_it->first);
            unsigned int n_bound_variables = bound_variables.size();
            // second pass: iterate through the possible states for the bound variables, adding a transition for each combination
            vector< vector<state> > inputs(n_inputs);
            state output;
            map<string,unsigned int> bound_variable_indices; // each is an index into vector<state> of 'variables' map
            for(unsigned int i=0;i<n_bound_variables;i++)
               bound_variable_indices[bound_variables[i]]=0;
            for(;;)
            {
               // output the transition for the current set of bound variables
               for(unsigned int i=0;i<n_inputs;i++) // collect the inputs
               {
                  if(!bound_variables.empty() && find(bound_variables.begin(),bound_variables.end(),tokens[i])!=bound_variables.end())
                    inputs[i] = vector<state>(1,variables[tokens[i]][bound_variable_indices[tokens[i]]]); // this input is a bound variable
                  else if(variables.find(tokens[i])!=variables.end())
                     inputs[i] = variables[tokens[i]]; // this input is an unbound variable
                  else 
                  {
                     unsigned int s;
                     if(sscanf(tokens[i].c_str(),"%d",&s)!=1) // this input is a state
                     {
                        ostringstream oss;
                        oss << "Error reading " << filename << " on line " << lineno << ": " << line;
                        return oss.str();
                     }
                     if(s<0 || s>=this->n_states)
                     {
                        ostringstream oss;
                        oss << "Error reading " << filename << " on line " << lineno << ": " << line << " - state out of range";
                        return oss.str();
                     }
                     inputs[i] = vector<state>(1,s);
                  }
               }
               // collect the output
               if(!bound_variables.empty() && 
                  find(bound_variables.begin(),bound_variables.end(),tokens[n_inputs])!=bound_variables.end())
                     output = variables[tokens[n_inputs]][bound_variable_indices[tokens[n_inputs]]];
               else 
               {
                  unsigned int s;
                  if(variables.find(tokens[n_inputs])!=variables.end() &&
                    variables[tokens[n_inputs]].size()==1)
                  {
                     // single-state variables are permitted as the output
                     s = variables[tokens[n_inputs]][0];
                  }
                  else if(sscanf(tokens[n_inputs].c_str(),"%d",&s)!=1) // if not a bound variable, output must be a state
                  {
                     ostringstream oss;
                     oss << "Error reading " << filename << " on line " << lineno << ": " << line;
                     return oss.str();
                  }
                  if(s<0 || s>=this->n_states)
                  {
                     ostringstream oss;
                     oss << "Error reading " << filename << " on line " << lineno << ": " << line << " - state out of range";
                     return oss.str();
                  }
                  output = s;
               }
               transition_table.push_back(make_pair(inputs,output));
               // move on to the next value of bound variables
               {
                  unsigned int iChanging=0;
                  for(;iChanging<n_bound_variables;iChanging++)
                  {
                     if(bound_variable_indices[bound_variables[iChanging]] < variables[bound_variables[iChanging]].size()-1)
                     {
                        bound_variable_indices[bound_variables[iChanging]]++;
                        break;
                     }
                     else
                     {
                        bound_variable_indices[bound_variables[iChanging]]=0;
                     }
                  }
                  if(iChanging>=n_bound_variables)
                     break;
               }
            }
         }
      }
   }
   if(!n_states_parsed || !neighborhood_parsed || !symmetries_parsed)
   {
      ostringstream oss;
      oss << "Error reading " << filename << ": one or more of n_states, neighborhood or symmetries missing";
      return oss.str();
   }
   // now convert transition table to bitmask lookup
   {
      unsigned int n_bits = sizeof(TBits)*8;
      int n_rotations,rotation_skip,n_reflections;
      vector<int> reflect_remap[2];
      if(this->neighborhood==vonNeumann)
      {
         reflect_remap[0].resize(5);
         reflect_remap[0][0]=0;
         reflect_remap[0][1]=1;
         reflect_remap[0][2]=2;
         reflect_remap[0][3]=3;
         reflect_remap[0][4]=4;
         reflect_remap[1].resize(5);
         reflect_remap[1][0]=0;
         reflect_remap[1][1]=1;
         reflect_remap[1][2]=4;
         reflect_remap[1][3]=3;
         reflect_remap[1][4]=2;  // we swap E and W
      }
      else // this->neighborhood==Moore
      {
         reflect_remap[0].resize(9);
         reflect_remap[0][0]=0;
         reflect_remap[0][1]=1;
         reflect_remap[0][2]=2;
         reflect_remap[0][3]=3;
         reflect_remap[0][4]=4;
         reflect_remap[0][5]=5;
         reflect_remap[0][6]=6;
         reflect_remap[0][7]=7;
         reflect_remap[0][8]=8;
         reflect_remap[1].resize(9);
         reflect_remap[1][0]=0;
         reflect_remap[1][1]=1;
         reflect_remap[1][2]=8;
         reflect_remap[1][3]=7;
         reflect_remap[1][4]=6;
         reflect_remap[1][5]=5;
         reflect_remap[1][6]=4;
         reflect_remap[1][7]=3;
         reflect_remap[1][8]=2; // all E and W swapped
      }
      switch(this->symmetries)
      {
         default:
         case none: n_rotations=1; rotation_skip=1; n_reflections=1; 
            break;
         case rotate4: n_rotations=4; n_reflections=1; 
            if(this->neighborhood==vonNeumann)
               rotation_skip=1;
            else // neighborhood==Moore
               rotation_skip=2;
            break;
         case rotate8: n_rotations=8; rotation_skip=1; n_reflections=1; 
            break;
         case reflect: n_rotations=1; rotation_skip=1; n_reflections=2; 
            break;
         case rotate4reflect: n_rotations=4; n_reflections=2; 
            if(this->neighborhood==vonNeumann)
               rotation_skip=1;
            else // neighborhood==Moore
               rotation_skip=2;
            break;
         case rotate8reflect: n_rotations=8; rotation_skip=1; n_reflections=2; 
            break;
      }
      unsigned int M = transition_table.size() * n_rotations * n_reflections; // (we need to expand out symmetry)
      this->n_compressed_rules = (M+n_bits-1) / n_bits; // the rule table is compressed down to 1 bit each
      // initialize lookup table to all bits turned off 
      this->lut.assign(n_inputs,vector< vector<TBits> >(this->n_states,vector<TBits>(this->n_compressed_rules,0))); 
      this->output.resize(M);
      // work through the rules, filling the bit masks
      unsigned int iRule=0,iRuleC,iBit,iNbor,iExpandedNbor;
      TBits mask;
      // (each transition rule looks like, e.g. 1,[2,3,5],4,[0,1],3 -> 0 )
      for(vector<pair<vector< vector<state> >,state> >::const_iterator rule_it = transition_table.begin();rule_it!=transition_table.end();rule_it++)
      {
         const vector< vector<state> >& rule_inputs = rule_it->first;
         for(int iRot=0;iRot<n_rotations;iRot++)
         {
            for(int iRef=0;iRef<n_reflections;iRef++)
            {
               this->output[iRule] = rule_it->second;
               iBit = iRule % n_bits;
               iRuleC = (iRule-iBit)/n_bits; // the compressed index of the rule
               mask = (TBits)1 << iBit; // (cast needed to ensure this is a 64-bit shift, not a 32-bit shift)
               for(iNbor=0;iNbor<n_inputs;iNbor++)
               {
                  const vector<state>& possibles = rule_inputs[iNbor];
                  for(vector<state>::const_iterator poss_it=possibles.begin();poss_it!=possibles.end();poss_it++)
                  {
                     // apply the necessary rotation to the non-centre cells
                     if(iNbor>0)
                        iExpandedNbor = 1+((iNbor-1+iRot*rotation_skip)%(n_inputs-1));
                     else
                        iExpandedNbor = iNbor;
                     // apply any reflection
                     iExpandedNbor = reflect_remap[iRef][iExpandedNbor];
                     // apply the resulting bit mask
                     this->lut[iExpandedNbor][*poss_it][iRuleC] |= mask;
                  }
               }
               iRule++; // this is the index of the rule after expansion for symmetry
            }
         }
      }
   }
   return string(""); // success
}

// --- the update function ---
state ruletable::slowcalc(state nw, state n, state ne, state w, state c, state e,
                          state sw, state s, state se) 
{
   unsigned int iRule;
   TBits is_match;

   for(iRule=0;iRule<this->n_compressed_rules;iRule++)
   {
      // is there a match for any of the (e.g.) 64 rules within iRule?
      // (we don't have to worry about symmetries here since they were expanded out in LoadRuleTable)
      if(this->neighborhood==vonNeumann)
         is_match = this->lut[0][c][iRule] & this->lut[1][n][iRule] & this->lut[2][e][iRule] & 
            this->lut[3][s][iRule] & this->lut[4][w][iRule];
      else // this->neighborhood==Moore
         is_match = this->lut[0][c][iRule] & this->lut[1][n][iRule] & this->lut[2][ne][iRule] & 
            this->lut[3][e][iRule] & this->lut[4][se][iRule] & this->lut[5][s][iRule] & this->lut[6][sw][iRule] & 
            this->lut[7][w][iRule] & this->lut[8][nw][iRule];
      // if any of them matched, return the output of the first
      if(is_match)
      {
         // find the least significant bit of is_match
         unsigned int iBit=0;
         TBits mask=1;
         while(!(is_match&mask))
         {
            ++iBit;
            mask <<= 1;
         }
         return this->output[ iRule*sizeof(TBits)*8 + iBit ];
      }
   }
   return c; // default: no change
}

