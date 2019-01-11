/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2019 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

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
#ifndef __PLUMED_tools_Grid_h
#define __PLUMED_tools_Grid_h

#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <memory>

namespace PLMD {


// simple function to enable various weighting

class WeightBase {
public:
  virtual double projectInnerLoop(double &input, double &v)=0;
  virtual double projectOuterLoop(double &v)=0;
  virtual ~WeightBase() {}
};

class BiasWeight:public WeightBase {
public:
  double beta,invbeta;
  explicit BiasWeight(double v) {beta=v; invbeta=1./beta;}
  double projectInnerLoop(double &input, double &v) {return  input+exp(beta*v);}
  double projectOuterLoop(double &v) {return -invbeta*std::log(v);}
};

class ProbWeight:public WeightBase {
public:
  double beta,invbeta;
  explicit ProbWeight(double v) {beta=v; invbeta=1./beta;}
  double projectInnerLoop(double &input, double &v) {return  input+v;}
  double projectOuterLoop(double &v) {return -invbeta*std::log(v);}
};






class Value;
class IFile;
class OFile;
class KernelFunctions;
class Communicator;

/// \ingroup TOOLBOX
class Grid
{
public:
// we use a size_t here
// should be 8 bytes on all 64-bit machines
// and more portable than "unsigned long long"
  typedef size_t index_t;
// to restore old implementation (unsigned) use the following instead:
// typedef unsigned index_t;
/// Maximum dimension (exaggerated value).
/// Can be used to replace local std::vectors with std::arrays (allocated on stack).
  static constexpr size_t maxdim=64;
private:
  double contour_location;
  std::vector<double> grid_;
  std::vector< std::vector<double> > der_;
protected:
  std::string funcname;
  std::vector<std::string> argnames;
  std::vector<std::string> str_min_, str_max_;
  std::vector<double> min_,max_,dx_;
  std::vector<unsigned> nbin_;
  std::vector<bool> pbc_;
  index_t maxsize_;
  unsigned dimension_;
  bool dospline_, usederiv_;
  std::string fmt_; // format for output
/// get "neighbors" for spline
  void getSplineNeighbors(const std::vector<unsigned> & indices, std::vector<index_t>& neigh, unsigned& nneigh )const;
// std::vector<index_t> getSplineNeighbors(const std::vector<unsigned> & indices)const;


public:
/// clear grid
  virtual void clear();
/// this constructor here is Value-aware
  Grid(const std::string& funcl, const std::vector<Value*> & args, const std::vector<std::string> & gmin,
       const std::vector<std::string> & gmax, const std::vector<unsigned> & nbin, bool dospline,
       bool usederiv, bool doclear=true);
/// this constructor here is not Value-aware
  Grid(const std::string& funcl, const std::vector<std::string> &names, const std::vector<std::string> & gmin,
       const std::vector<std::string> & gmax, const std::vector<unsigned> & nbin, bool dospline,
       bool usederiv, bool doclear, const std::vector<bool> &isperiodic, const std::vector<std::string> &pmin,
       const std::vector<std::string> &pmax );
/// this is the real initializator
  void Init(const std::string & funcl, const std::vector<std::string> &names, const std::vector<std::string> & gmin,
            const std::vector<std::string> & gmax, const std::vector<unsigned> & nbin, bool dospline, bool usederiv,
            bool doclear, const std::vector<bool> &isperiodic, const std::vector<std::string> &pmin, const std::vector<std::string> &pmax);
/// get lower boundary
  std::vector<std::string> getMin() const;
/// get upper boundary
  std::vector<std::string> getMax() const;
/// get bin size
  std::vector<double> getDx() const;
  double getDx(index_t j) const ;
/// get bin volume
  double getBinVolume() const;
/// get number of bins
  std::vector<unsigned> getNbin() const;
/// get if periodic
  std::vector<bool> getIsPeriodic() const;
/// get grid dimension
  unsigned getDimension() const;
/// get argument names  of this grid
  std::vector<std::string> getArgNames() const;
/// get if the grid has derivatives
  bool hasDerivatives() const {return usederiv_;}

/// methods to handle grid indices
  void getIndices(index_t index, std::vector<unsigned>& rindex) const;
  void getIndices(const std::vector<double> & x, std::vector<unsigned>& rindex) const;
  std::vector<unsigned> getIndices(index_t index) const;
  std::vector<unsigned> getIndices(const std::vector<double> & x) const;
  index_t getIndex(const std::vector<unsigned> & indices) const;
  index_t getIndex(const std::vector<double> & x) const;
  std::vector<double> getPoint(index_t index) const;
  std::vector<double> getPoint(const std::vector<unsigned> & indices) const;
  std::vector<double> getPoint(const std::vector<double> & x) const;
/// faster versions relying on preallocated vectors
  void getPoint(index_t index,std::vector<double> & point) const;
  void getPoint(const std::vector<unsigned> & indices,std::vector<double> & point) const;
  void getPoint(const std::vector<double> & x,std::vector<double> & point) const;

/// get neighbors
  std::vector<index_t> getNeighbors(index_t index,const std::vector<unsigned> & neigh) const;
  std::vector<index_t> getNeighbors(const std::vector<unsigned> & indices,const std::vector<unsigned> & neigh) const;
  std::vector<index_t> getNeighbors(const std::vector<double> & x,const std::vector<unsigned> & neigh) const;
/// get nearest neighbors (those separated by exactly one lattice unit)
  std::vector<index_t> getNearestNeighbors(const index_t index) const;
  std::vector<index_t> getNearestNeighbors(const std::vector<unsigned> &indices) const;

/// write header for grid file
  void writeHeader(OFile& file);

/// read grid from file
  static std::unique_ptr<Grid> create(const std::string&,const std::vector<Value*>&,IFile&,bool,bool,bool);
/// read grid from file and check boundaries are what is expected from input
  static std::unique_ptr<Grid> create(const std::string&,const std::vector<Value*>&, IFile&,
                                      const std::vector<std::string>&,const std::vector<std::string>&,
                                      const std::vector<unsigned>&,bool,bool,bool);
/// get grid size
  virtual index_t getSize() const;
/// get grid value
  virtual double getValue(index_t index) const;
  virtual double getValue(const std::vector<unsigned> & indices) const;
  virtual double getValue(const std::vector<double> & x) const;
/// get minimum value
  virtual double getMinValue() const;
/// get maximum value
  virtual double getMaxValue() const;
/// get grid value and derivatives
  virtual double getValueAndDerivatives(index_t index, std::vector<double>& der) const ;
  virtual double getValueAndDerivatives(const std::vector<unsigned> & indices, std::vector<double>& der) const;
  virtual double getValueAndDerivatives(const std::vector<double> & x, std::vector<double>& der) const;
/// Get the difference from the contour
  double getDifferenceFromContour(const std::vector<double> & x, std::vector<double>& der) const ;
/// Find a set of points on a contour in the function
  void findSetOfPointsOnContour(const double& target, const std::vector<bool>& nosearch, unsigned& npoints, std::vector<std::vector<double> >& points );

/// set grid value
  virtual void setValue(index_t index, double value);
  virtual void setValue(const std::vector<unsigned> & indices, double value);
/// set grid value and derivatives
  virtual void setValueAndDerivatives(index_t index, double value, std::vector<double>& der);
  virtual void setValueAndDerivatives(const std::vector<unsigned> & indices, double value, std::vector<double>& der);
/// add to grid value
  virtual void addValue(index_t index, double value);
  virtual void addValue(const std::vector<unsigned> & indices, double value);
/// add to grid value and derivatives
  virtual void addValueAndDerivatives(index_t index, double value, std::vector<double>& der);
  virtual void addValueAndDerivatives(const std::vector<unsigned> & indices, double value, std::vector<double>& der);
/// Scale all grid values and derivatives by a constant factor
  virtual void scaleAllValuesAndDerivatives( const double& scalef );
/// Takes the scalef times the logarithm of all grid values and derivatives
  virtual void logAllValuesAndDerivatives( const double& scalef );
/// Set the minimum value of the grid to zero and translates accordingly
  virtual void setMinToZero();
/// apply function: takes  pointer to  function that accepts a double and apply
  virtual void applyFunctionAllValuesAndDerivatives( double (*func)(double val), double (*funcder)(double valder) );
/// add a kernel function to the grid
  void addKernel( const KernelFunctions& kernel );

/// dump grid on file
  virtual void writeToFile(OFile&);
/// dump grid to gaussian cube file
  void writeCubeFile(OFile&, const double& lunit);

  virtual ~Grid() {}

/// project a high dimensional grid onto a low dimensional one: this should be changed at some time
/// to enable many types of weighting
  Grid project( const std::vector<std::string> & proj, WeightBase *ptr2obj  );
  void projectOnLowDimension(double &val, std::vector<int> &varHigh, WeightBase* ptr2obj );
/// set output format
  void setOutputFmt(const std::string & ss) {fmt_=ss;}
/// reset output format to the default %14.9f format
  void resetToDefaultOutputFmt() {fmt_="%14.9f";}
/// Integrate the function calculated on the grid
  double integrate( std::vector<unsigned>& npoints );
///
  void mpiSumValuesAndDerivatives( Communicator& comm );
/// Find the maximum over paths of the minimum value of the gridded function along the paths
/// for all paths of neighboring grid lattice points from a source point to a sink point.
  virtual double findMaximalPathMinimum(const std::vector<double> &source, const std::vector<double> &sink);
};


class SparseGrid : public Grid
{

  std::map<index_t,double> map_;
  std::map< index_t,std::vector<double> > der_;

protected:
  void clear();

public:
  SparseGrid(const std::string& funcl, const std::vector<Value*> & args, const std::vector<std::string> & gmin,
             const std::vector<std::string> & gmax,
             const std::vector<unsigned> & nbin, bool dospline, bool usederiv):
    Grid(funcl,args,gmin,gmax,nbin,dospline,usederiv,false) {}

  index_t getSize() const;
  index_t getMaxSize() const;

/// this is to access to Grid:: version of these methods (allowing overloading of virtual methods)
  using Grid::getValue;
  using Grid::getValueAndDerivatives;
  using Grid::setValue;
  using Grid::setValueAndDerivatives;
  using Grid::addValue;
  using Grid::addValueAndDerivatives;

/// get grid value
  double getValue(index_t index) const;
/// get grid value and derivatives
  double getValueAndDerivatives(index_t index, std::vector<double>& der) const;

/// set grid value
  void setValue(index_t index, double value);
/// set grid value and derivatives
  void setValueAndDerivatives(index_t index, double value, std::vector<double>& der);
/// add to grid value
  void addValue(index_t index, double value);
/// add to grid value and derivatives
  void addValueAndDerivatives(index_t index, double value, std::vector<double>& der);

/// dump grid on file
  void writeToFile(OFile&);

  virtual ~SparseGrid() {}
};
}

#endif
