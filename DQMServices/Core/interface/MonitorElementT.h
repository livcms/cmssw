#ifndef MonitorElementT_h
#define MonitorElementT_h

#include "DQMServices/Core/interface/MonitorElement.h"

#include <iostream>

template<class T>
class MonitorElementT : public MonitorElement
{
 public:
  MonitorElementT(T *val, const std::string name="") : 
  name_(name), val_(val) 
  {reference_ = 0;}
  virtual ~MonitorElementT() 
  {
    delete val_;
    if(reference_)deleteReference();
  }

  void clear(){} 
  // pointer to val_
  T * operator->(){
    update();
    return val_;
  }
  // const pointer to val_
  const T * const_ptr() const
  {
    return (const T *) val_;
  }

  // return *val_ by reference
  T & operator*()
    {
      update();
      return *val_;
    }
  // return *val_ by value
  T operator*() const {return *val_;}
  
  // noarg functor (do we need this?)
  T operator()(){return *val_;}

  // explicit cast overload
  operator T(){return *val_;}

  virtual std::string getName() const {return name_;}
  virtual T getValue() const {return *val_;}

  virtual void Reset()=0;

  virtual std::string valueString() const {return std::string();};

  float doNotUseMethod(std::string method) const
  {
    std::cerr << " *** Cannot use method " << method << 
      " with MonitorElement " << getName() << std::endl;
    return -999;
  }

  // mean value of histogram along x, y or z axis (axis=1, 2, 3 respectively)
  virtual float getMean(int axis = 1) const
  {return doNotUseMethod("getMean");}
  // mean value uncertainty of histogram along x, y or z axis 
  // (axis=1, 2, 3 respectively)
  virtual float getMeanError(int axis = 1) const
  {return doNotUseMethod("getMeanError");}
  // RMS of histogram along x, y or z axis (axis=1, 2, 3 respectively)
  virtual float getRMS(int axis = 1) const
  {return doNotUseMethod("getRMS");}
  // RMS uncertainty of histogram along x, y or z axis (axis=1, 2, 3 respectively)
  virtual float getRMSError(int axis = 1) const
  {return doNotUseMethod("getRMSError");}
  // content of bin (1-D)
  virtual float getBinContent(int binx) const
  {return doNotUseMethod("getBinContent(binx)");}
  // content of bin (2-D)
  virtual float getBinContent(int binx, int biny) const
  {return doNotUseMethod("getBinContent(binx,biny)");}
  // content of bin (3-D)
  virtual float getBinContent(int binx, int biny, int binz) const
  {return doNotUseMethod("getBinContent(binx,biny,binz)");}
  // uncertainty on content of bin (1-D) - See TH1::GetBinError for details
  virtual float getBinError(int binx) const
  {return doNotUseMethod("getBinError(binx)");}
  // uncertainty on content of bin (2-D) - See TH1::GetBinError for details
  virtual float getBinError(int binx, int biny) const
  {return doNotUseMethod("getBinError(binx,biny)");}
  // uncertainty on content of bin (3-D) - See TH1::GetBinError for details
  virtual float getBinError(int binx, int biny, int binz) const
  {return doNotUseMethod("getBinError(binx,biny,binz)");}
  // # of entries
  virtual float getEntries(void){return 1;}
  // # of bin entries (for profiles)
  virtual float getBinEntries(int bin)
  {return doNotUseMethod("getBinEntries");}
 private:
  
  std::string name_;
  T * val_;
 protected:

  // make sure axis is one of 1 (x), 2 (y) or 3 (z)
  bool checkAxis(int axis) const
  {
    if (axis < 1 || axis > 3) return false;
    return true;
  }

  T * reference_; // this is set to "val_" upon a "softReset"

  // delete reference_
  void deleteReference(void)
  {
    if(!reference_)
      {
	std::cerr << " *** Cannot delete null reference for " 
		  << getName() << std::endl;
	return;
      }
    delete reference_;
    reference_ = 0;
  }

  // for description: see DQMServices/Core/interface/MonitorElement.h
  void enableSoftReset(bool flag)
  {
    softReset_on = flag;

    std::cout << " \"soft-reset\" option has been";
    if(softReset_on)
      std::cout << " en";
    else
      std::cout << " dis";
    std::cout << "abled for " << getName() << std::endl;

    if(softReset_on)
	this->softReset();
    else
      if(reference_)deleteReference();
    
  }

  // this is really bad; unfortunately, gcc 3.2.3 won't let me define 
  // template classes, so I have to find a workaround for now
  // error: "...is not a template type" - christos May26, 2005
  friend class CollateMERootH1;
  friend class CollateMERootH2;
  friend class CollateMERootH3;
  friend class CollateMERootProf;
  friend class CollateMERootProf2D;

};
#endif









