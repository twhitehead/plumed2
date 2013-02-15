/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2012 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

   This file is part of plumed, version 2.0.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "FunctionVessel.h"
#include "core/ActionWithValue.h"

namespace PLMD{
namespace vesselbase{

void FunctionVessel::registerKeywords( Keywords& keys ){
  Vessel::registerKeywords( keys );
  plumed_assert( keys.size()==0 );
}

FunctionVessel::FunctionVessel( const VesselOptions& da ):
Vessel(da)
{
  ActionWithValue* a=dynamic_cast<ActionWithValue*>( getAction() );
  plumed_massert(a,"cannot create passable values as base action does not inherit from ActionWithValue");
  int numval = getNumericalLabel();
  if( numval<0 ){   // This allows us to make multicolvars pretend to be colvars - this is used in AlphaRMSD etc
     plumed_massert( a->getNumberOfComponents()==0,"you can't multiple values with the action label");
     a->addValueWithDerivatives();  a->setNotPeriodic();
  } else {
     plumed_massert( !a->exists(getAction()->getLabel() + "." + getLabel() ), "you can't create the name multiple times");
     a->addComponentWithDerivatives( getLabel() ); a->componentIsNotPeriodic( getLabel() );
  }
  final_value=a->copyOutput( a->getNumberOfComponents()-1 );
  diffweight=getAction()->weightHasDerivatives;
}

std::string FunctionVessel::description(){
  return "value " + getAction()->getLabel() + "." + getLabel() + " contains " + function_description();
}

void FunctionVessel::resize(){
  nderivatives=getAction()->getNumberOfDerivatives();
  resizeBuffer( (1+nderivatives)*getNumberOfTerms() ); 
  final_value->resizeDerivatives( nderivatives );
}

void FunctionVessel::mergeFinalDerivatives( const std::vector<double>& df ){
  plumed_dbg_assert( df.size()==getNumberOfTerms() ); 
  for(unsigned i=0;i<getNumberOfTerms();++i){
      unsigned ider=i*( nderivatives + 1 ) + 1;
      for(unsigned k=0;k<nderivatives;++k){
         final_value->addDerivative( k, df[i]*getBufferElement( ider ) ); ider++;
      }
  }
}

bool FunctionVessel::applyForce( std::vector<double>& forces ){
  std::vector<double> tmpforce( forces.size() );
  forces.assign(forces.size(),0.0); bool wasforced=false;
  if( final_value->applyForce( tmpforce ) ){
      wasforced=true;
      for(unsigned j=0;j<forces.size();++j) forces[j]+=tmpforce[j];
  }
  return wasforced;
}

}
}

