#ifndef CDIST_H
#define CDIST_H
#include <cmath>
#include <vector>
#include "CMatrix.h"
#include "CTransform.h"
#include "ndlutil.h"
              

const string DISTVERSION="0.1";

// Base distribution class.
class CDist : public CMatinterface, public CTransformable {
  
 public:
  CDist(){}
  virtual ~CDist(){}
  int getNumParams() const
    {
      return nParams;
    }
  void setNumParams(int num)
    {
      nParams = num;
    }
  virtual double getParam(int paramNo) const=0;
  virtual void setParam(double val, int paramNo)=0;
  virtual void getGradParams(CMatrix& g) const
    {
      // This is a dummy function
      cerr << "getGradParams should not be used in CDist" << endl;
      exit(1);
    }

#ifdef _NDLMATLAB
  // returns an mxArray of the dist for use with matlab.
  virtual mxArray* toMxArray() const;
  virtual void fromMxArray(const mxArray* matlabArray);
  // Adds parameters to the mxArray.
  virtual void addParamToMxArray(mxArray* matlabArray) const;
  // Gets the parameters from the mxArray.
  virtual void extractParamFromMxArray(const mxArray* matlabArray);
#endif /* _NDLMATLAB*/

  virtual void writeParamsToStream(ostream& out) const;
  virtual void readParamsFromStream(istream& in);
  //CDist(CDist& dist);
  // get the gradient with respect to an input.
  virtual double getGradInput(double x) const=0;
  // get the gradient with respect to a matrix of inputs
  virtual void getGradInputs(CMatrix& g, const CMatrix& x)
    {
      assert(g.getRows()==x.getRows());
      assert(g.getCols()==x.getCols());
      for(int i=0; i<g.getRows(); i++)
	for(int j=0; j<g.getCols(); j++)
	  g.setVal(getGradInput(x.getVal(i, j)), i, j);
    }
  void setInitParam();
  // Get log probability at a particualar value
  virtual double logProb(double val) const=0;
  virtual double logProb(const CMatrix& x)
    {
      double ll = 0.0;
      for(int i=0; i<x.getRows(); i++)
	for(int j=0; j<x.getCols(); j++)
	  ll+=logProb(x.getVal(i, j));
      return ll;
    }
  void setParamName(const string name, int index)
    {
      assert(index>=0);
      assert(index<nParams);
      if(paramNames.size() == index)
	paramNames.push_back(name);
      else 
	{
	  if(paramNames.size()<index)
	    paramNames.resize(index+1, "no name");
	  paramNames[index] = name;
	}
    }
  virtual string getParamName(int index) const
    {
      assert(index>=0);
      assert(index<paramNames.size());
      return paramNames[index];
    }

  void setType(string name)
    {
      type = name;
    }
  string getType() const
    {
      return type;
    }
  void setName(string name)
    {
      distName = name;
    }
  string getName() const
    {
      return distName;
    }
  
 private:
  int nParams;
  string type;
  string distName;
  vector<string> paramNames;
};

// Read and write dists to streams
void writeDistToStream(const CDist& dist, ostream& out);
CDist* readDistFromStream(istream& in);

// The Gaussian distribution.
class CGaussianDist : public CDist {

 public:
  CGaussianDist();
  CGaussianDist(const CGaussianDist&);
  ~CGaussianDist();
  CGaussianDist* clone() const
    {
      return new CGaussianDist(*this);
    }
  double getParam(int paramNo) const;
  void setParam(double val, int paramNo);
  double getGradInput(double x) const;
  void setInitParam();
  double logProb(double val) const;

 private:
  double precision;
};

// The gamma distribution
class CGammaDist : public CDist {

 public:
  CGammaDist();
  CGammaDist(const CGammaDist&);
  ~CGammaDist();
  CGammaDist* clone() const
    {
      return new CGammaDist(*this);
    }
  double getParam(int paramNo) const;
  void setParam(double val, int paramNo);
  double getGradInput(double x) const;
  void setInitParam();
  double logProb(double val) const;

 private:
  double a;
  double b;
};
// A class which stores distributions in a container for priors over parameters.
// An unusual prior used by Wang in the GPDM thesis.
class CWangDist : public CDist {

 public:
  CWangDist();
  CWangDist(const CWangDist&);
  ~CWangDist();
  CWangDist* clone() const
    {
      return new CWangDist(*this);
    }
  double getParam(int paramNo) const;
  void setParam(double val, int paramNo);
  double getGradInput(double x) const;
  void setInitParam();
  double logProb(double val) const;

 private:
  double M;
};
// A class which stores distributions in a container for priors over parameters.
class CParamPriors : CMatinterface {
  
 public:
#ifdef _NDLMATLAB
  mxArray* toMxArray() const;
  void fromMxArray(const mxArray* distArray);
#endif
  void addDist(CDist* dist, int index)
    {
      assert(index>=0);
      distIndex.push_back(index);
      dists.push_back(dist);
    }

  void clearDists()
    {
      distIndex.clear();
      dists.clear();
    }
  inline string getDistType(int ind) const
    {
      assert(ind>=0);
      assert(ind<getNumDists());
      return dists[ind]->getType();
    }
  inline int getDistIndex(int ind) const
    {
      assert(ind>=0);
      assert(ind<getNumDists());
      return distIndex[ind];
    }
  inline int getNumDists() const
    {
      return dists.size();
    }
  vector<CDist*> dists;
  vector<int> distIndex;

};

// A virtual base class which makes its descendents regluarisable.
class CRegularisable {

 public:
   virtual ~CRegularisable() {}

  // these are the pure virtual functions.
  virtual int getNumParams() const=0;
  virtual double getParam(int paramNo) const=0;
  virtual void setParam(double val, int paramNo)=0;
  virtual void getGradParams(CMatrix& g) const=0;

  // these are default implementations.
  virtual void getParams(CMatrix& params) const
    {
      assert(params.getRows()==1);
      assert(params.getCols()==getNumParams());
      for(int i=0; i<params.getCols(); i++)
	params.setVal(getParam(i), i);
    }
  virtual void setParams(const CMatrix& params)
    {
      assert(params.getRows()==1);
      assert(params.getCols()==getNumParams());
      for(int i=0; i<params.getCols(); i++)
	setParam(params.getVal(i), i);
    }
  
  virtual void addPriorGrad(CMatrix& g) const
    {
      assert(g.getRows()==1);
      assert(g.getCols()==getNumParams());
      double param=0.0;
      for(int i=0; i<distArray.distIndex.size(); i++)
	{
	  param=getParam(distArray.distIndex[i]);
	  g.addVal(getPriorGradInput(param, i), 
		   distArray.distIndex[i]);
	}  
    }
      
  virtual void writePriorsToStream(ostream& out) const
    {
      for(int i=0; i<distArray.distIndex.size(); i++)
	{
	  out << "priorIndex=" << distArray.distIndex[i] << endl;
	  writeDistToStream(*distArray.dists[i], out);
	}
    }
  virtual void readPriorsFromStream(istream& in, int numPriors)
    {
      string line;
      vector<string> tokens;
      for(int i=0; i<numPriors; i++)
	{
	  CDist* prior;
	  getline(in, line);
	  ndlstrutil::tokenise(tokens, line, "=");
	  if(tokens.size()>2 || tokens[0]!="priorIndex")
	    throw ndlexceptions::FileFormatError();
	  prior = readDistFromStream(in);
	  addPrior(prior, atol(tokens[1].c_str()));
	  tokens.clear();
	}
    }

  virtual double priorLogProb() const
    {
      double L = 0.0;
      double param=0.0;
      for(int i=0; i<distArray.distIndex.size(); i++)
	{
	  param = getParam(distArray.distIndex[i]);
	  L+=distArray.dists[i]->logProb(param);
	}
      return L;
    }

  // These are non-modifiable methods.
  inline int getNumPriors() const
    {
      return distArray.getNumDists();
    }
  inline CDist* getPrior(int ind) const
    {
      assert(ind>=0);
      assert(ind<getNumPriors());
      return distArray.dists[ind];
    }
  inline string getPriorType(int ind) const
    {
      return distArray.getDistType(ind);
    }
  inline int getPriorIndex(int ind) const
    {
      return distArray.getDistIndex(ind);
    }
  inline double getPriorGradInput(double val, int ind) const
    {
      return distArray.dists[ind]->getGradInput(val);
    }
  void addPrior(CDist* dist, int index)
    {
      assert(index>=0);
      assert(index<getNumParams());
      distArray.distIndex.push_back(index);
      distArray.dists.push_back(dist);
    }

  void clearPriors()
    {
      distArray.distIndex.clear();
      distArray.dists.clear();
    }
#ifdef _NDLMATLAB
  mxArray* distsToMxArray() const
    {
      return distArray.toMxArray();
    }
  void distsFromMxArray(const mxArray* matlabArray) 
    {
      distArray.fromMxArray(matlabArray);
    }
#endif
 private:
  CParamPriors distArray;

};



#endif

    
