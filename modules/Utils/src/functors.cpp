//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Functor class definitions.
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///   
///  \author Pat Scott 
///          (patscott@physics.mcgill.ca)
///  \date 2013 Apr-July, Dec
///  \date 2014 Jan, Mar-May, Sep
///
///  \author Anders Kvellestad
///          (anders.kvellestad@fys.uio.no) 
///   \date 2013 Apr, Nov
///
///  \author Christoph Weniger
///          (c.weniger@uva.nl)
///  \date 2013 May, June, July
///
///  \author Ben Farmer
///          (benjamin.farmer@monash.edu.au)
///  \date 2013 July, Sep
///  \date 2014 Jan
///
///  *********************************************

#include <chrono>

#include "functors.hpp"
#include "models.hpp"
#include "all_functor_types.hpp"
#include "standalone_error_handlers.hpp"
#include "log.hpp"

#include <boost/preprocessor/seq/for_each.hpp>

#define FWDPRINT(r,data,elem) virtual void print(elem const&, const std::string&, const int) = 0;

namespace Gambit
{
  using namespace LogTags;

  /// Poorly declaration of Printers::BasePrinter for use in print functions; leave implementation to printer source files.
  /// If the compiled printer sources are not linked against the compiled program, any calls to the print functions will generate
  /// link errors.  This is how printers are removed from standalone module compilation, and calls to the print functions outlawed.
  namespace Printers
  {
    class BasePrinter
    {
      public:
        BOOST_PP_SEQ_FOR_EACH(FWDPRINT, _, PRINTABLE_TYPES)
        virtual void initialise(const std::vector<int>&) = 0;
        virtual void endline() = 0;
    };
  }

   
  // Functor class methods

    /// Constructor
    functor::functor (str func_name,
                      str func_capability,
                      str result_type,
                      str origin_name,
                      Models::ModelFunctorClaw &claw) :      
     myName          (func_name),
     myCapability    (func_capability),
     myType          (Utils::fix_type(result_type)),
     myOrigin        (origin_name),
     myClaw          (&claw),
     myLabel         (func_capability+" -- "+origin_name+"::"+func_name),
     myStatus        (0),
     myVertexID      (-1),       // (Note: myVertexID = -1 is intended to mean that no vertexID has been assigned)
     verbose         (false),    // For debugging.
     needs_recalculating (true)
    {
       std::stringstream ss;
       ss<<"#"<<capability()<<" @"<<origin()<<"::"<<name(); 
       setLabel(ss.str());
    }
    
    /// Virtual calculate(); needs to be redefined in daughters.
    void functor::calculate() {}

    /// Interfaces for runtime optimization
    /// Need to be implemented by daughters
    /// @{
    double functor::getRuntimeAverage() { return 0; }
    double functor::getInvalidationRate() { return 0; }
    void functor::setFadeRate() {}
    void functor::notifyOfInvalidation(const str&) {}
    void functor::reset() {}
    /// @}

    /// Reset-then-recalculate method
    void functor::reset_and_calculate() { this->reset(); this->calculate(); } 

    /// Setter for purpose (relevant only for next-to-output functors)
    void functor::setPurpose(str purpose) { if (this == NULL) failBigTime("setPurpose"); myPurpose = purpose; }

    /// Setter for vertex ID (used in printer system)
    void functor::setVertexID(int vertexID) { if (this == NULL) failBigTime("setVertexID"); myVertexID = vertexID; }
    
    /// Setter for status (-2 = function absent, -1 = origin absent, 0 = model incompatibility (default), 1 = available, 2 = active)
    void functor::setStatus(int stat)
    {
      if (this == NULL) failBigTime("setStatus");
      myStatus = stat;
      setInUse(myStatus == 2);       
    }

    /// Setter for label (used in printer system)
    void functor::setLabel(str label) { if (this == NULL) failBigTime("setLabel"); myLabel = label; } 
   
    /// Getter for the wrapped function's name
    str functor::name()        const { if (this == NULL) failBigTime("name"); return myName; }
    /// Getter for the wrapped function's reported capability
    str functor::capability()  const { if (this == NULL) failBigTime("capability"); return myCapability; }
    /// Getter for the wrapped function's reported return type
    str functor::type()        const { if (this == NULL) failBigTime("type"); return myType; }
    /// Getter for the wrapped function's origin (module or backend name)
    str functor::origin()      const { if (this == NULL) failBigTime("origin"); return myOrigin; }
    /// Getter for the version of the wrapped function's origin (module or backend)
    str functor::version()     const { if (this == NULL) failBigTime("version"); return myVersion; }
    /// Getter for the 'safe' incarnation of the version of the wrapped function's origin (module or backend)
    str functor::safe_version()const { utils_error().raise(LOCAL_INFO,"The safe_version method is only defined for backend functors."); return ""; }
    /// Getter for the wrapped function current status (-3 = required classes absent, -2 = function absent, -1 = origin absent, 0 = model incompatibility (default), 1 = available, 2 = active)
    int functor::status()      const { if (this == NULL) failBigTime("status"); return myStatus; }
    /// Getter for the  overall quantity provided by the wrapped function (capability-type pair)
    sspair functor::quantity() const { if (this == NULL) failBigTime("quantity"); return std::make_pair(myCapability, myType); }
    /// Getter for purpose (relevant for output nodes, aka helper structures for the dep. resolution)
    str functor::purpose()     const { if (this == NULL) failBigTime("purpose"); return myPurpose; }
    /// Getter for vertex ID
    int functor::vertexID()    const { if (this == NULL) failBigTime("vertexID"); return myVertexID; }   
    /// Getter indicating if the wrapped function's result should to be printed
    bool functor::requiresPrinting() const { if (this == NULL) failBigTime("requiresPrinting"); return false; }
    /// Getter for the printer label
    str functor::label()       const { if (this == NULL) failBigTime("label"); return myLabel; }

    /// Setter for indicating if the wrapped function's result should to be printed
    void functor::setPrintRequirement(bool flag)
    {
      if (flag)
      {
        utils_error().raise(LOCAL_INFO,"The setPrintRequirement method has not been defined in this class.");
      }
    }

    /// Set the ordered list of pointers to other functors that should run nested in a loop managed by this one
    void functor::setNestedList (std::vector<functor*>&)
    { 
      utils_error().raise(LOCAL_INFO,"The setNestedList method has not been defined in this class.");
    } 

    /// Set the iteration number in a loop in which this functor runs
    void functor::setIteration (int)
    { 
      utils_error().raise(LOCAL_INFO,"The setIteration method has not been defined in this class.");
    }

    /// Getter for revealing whether this is permitted to be a manager functor
    bool functor::canBeLoopManager()
    {
      utils_error().raise(LOCAL_INFO,"The canBeLoopManager method has not been defined in this class.");
      return false;
    }

    /// Getter for revealing the required capability of the wrapped function's loop manager
    str functor::loopManagerCapability()
    {
      utils_error().raise(LOCAL_INFO,"The loopManagerCapability method has not been defined in this class.");
      return "none";
    }

    /// Getter for listing currently activated dependencies
    std::vector<sspair> functor::dependencies()          
    { 
      utils_error().raise(LOCAL_INFO,"The dependencies method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }
    /// Getter for listing backend requirement groups
    std::vector<str> functor::backendgroups()                   
    {
      utils_error().raise(LOCAL_INFO,"The backendgroups method has not been defined in this class.");
      std::vector<str> empty;
      return empty;
    }
    /// Getter for listing all backend requirements
    std::vector<sspair> functor::backendreqs()                   
    {
      utils_error().raise(LOCAL_INFO,"The backendreqs method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }
    /// Getter for listing backend requirements from a specific group
    std::vector<sspair> functor::backendreqs(str)                   
    {
      utils_error().raise(LOCAL_INFO,"The backendreqs method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }
    /// Getter for listing permitted backends
    std::vector<sspair> functor::backendspermitted(sspair) 
    { 
      utils_error().raise(LOCAL_INFO,"The backendspermitted method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }
    /// Getter for listing tags associated with backend requirements
    std::vector<str> functor::backendreq_tags(sspair)
    { 
      utils_error().raise(LOCAL_INFO,"The backendreq_tags method has not been defined in this class.");
      std::vector<str> empty;
      return empty;
    }
    /// Getter for listing backend requirements that must be resolved from the same backend
    std::vector<sspair> functor::forcematchingbackend(str)
    { 
      utils_error().raise(LOCAL_INFO,"The forcematchingbackend method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }

    /// Getter for listing backend-specific conditional dependencies (4-string version)
    std::vector<sspair> functor::backend_conditional_dependencies (str, str, str, str)  
    { 
      utils_error().raise(LOCAL_INFO,"The backend_conditional_dependencies method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }
    
    /// Tell the functor that the loop it manages should break now.
    void functor::breakLoop()
    { 
      utils_error().raise(LOCAL_INFO,"The breakLoop method has not been defined in this class.");
    }

    /// Getter for backend-specific conditional dependencies (3-string version)
    std::vector<sspair> functor::backend_conditional_dependencies (str req, str type, str be)  
    { 
      return backend_conditional_dependencies(req, type, be, "any");
    }
    
    /// Getter for backend-specific conditional dependencies (backend functor pointer version)
    std::vector<sspair> functor::backend_conditional_dependencies (functor* be_functor)  
    { 
      return backend_conditional_dependencies (be_functor->capability(), be_functor->type(),
       be_functor->origin(), be_functor->version());
    }

    /// Getter for listing model-specific conditional dependencies
    std::vector<sspair> functor::model_conditional_dependencies (str)
    { 
      utils_error().raise(LOCAL_INFO,"The model_conditional_dependencies method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }

    /// Getter for listing model-specific conditional backend requirements
    std::vector<sspair> functor::model_conditional_backend_reqs (str)
    { 
      utils_error().raise(LOCAL_INFO,"The model_conditional_backend_reqs method has not been defined in this class.");
      std::vector<sspair> empty;
      return empty;
    }

    /// Resolve a dependency using a pointer to another functor object
    void functor::resolveDependency (functor*)
    {
      utils_error().raise(LOCAL_INFO,"The resolveDependency method has not been defined in this class.");
    }

    // Set this functor's loop manager (if it has one)
    void functor::resolveLoopManager (functor*)
    {
      utils_error().raise(LOCAL_INFO,"The resolveLoopManager method has not been defined in this class.");
    }

    /// Resolve a backend requirement using a pointer to another functor object
    void functor::resolveBackendReq (functor*)
    {
      utils_error().raise(LOCAL_INFO,"The resolveBackendReq method has not been defined in this class.");
    }

    /// Notify the functor that a certain model is being scanned, so that it can activate its dependencies accordingly.
    void functor::notifyOfModel(str)
    {
      utils_error().raise(LOCAL_INFO,"The notifyOfModel method has not been defined in this class.");
    }

    /// Indicate to the functor which backends are actually loaded and working
    void functor::notifyOfBackends(std::map<str, std::set<str> >)
    {
      utils_error().raise(LOCAL_INFO,"The notifyOfBackends method has not been defined in this class.");
    }

    /// Notify the functor about an instance of the options class that contains
    /// information from its corresponding ini-file entry in the auxiliaries or
    /// observables section.
    void functor::notifyOfIniOptions(const Options& opt)
    {
      myOptions = opt;
    }

    /// Return a safe pointer to the options that this functor is supposed to run with (e.g. from the ini file).
    safe_ptr<Options> functor::getOptions()
    {
      if (this == NULL) functor::failBigTime("getOptions");
      return safe_ptr<Options>(&myOptions);       
    }

    /// Test whether the functor is allowed (either explicitly or implicitly) to be used with a given model
    bool functor::modelAllowed(str model)
    {
      if (allowedModels.empty() and allowedGroupCombos.empty()) return true;
      if (allowed_parent_or_friend_exists(model)) return true;
      return false;        
    }

    /// Test whether the functor has been explictly allowed to be used with a given model 
    bool functor::modelExplicitlyAllowed(str model)
    {
      if (allowedModels.find(model) != allowedModels.end()) return true;
      return false;
    }

    /// Test whether the functor is allowed to be used with all models
    bool functor::allModelsAllowed()
    {
      return allowedModels.empty() and allowedGroupCombos.empty();
    }

    /// Add a model to the internal list of models for which this functor is allowed to be used.
    void functor::setAllowedModel(str model) { allowedModels.insert(model); }

    /// Test whether the functor is allowed (either explicitly or implicitly) to be used with a given combination of models
    bool functor::modelComboAllowed(std::vector<str> combo)
    {
      // If any model in the combo is always allowed, then give the combo a thumbs up.
      for(std::vector<str>::const_iterator model = combo.begin(); model != combo.end(); model++)
      {
        if (modelAllowed(*model)) return true;
      }
      // Loop over the allowed combinations, and check if the passed combo matches any of them
      for(std::set<std::vector<str> >::const_iterator group_combo = allowedGroupCombos.begin(); group_combo != allowedGroupCombos.end(); group_combo++)
      {  
        bool matches = true;
        //Loop over each group in the allowed group combination, and check if one of the entries in the passed model combination matches it somehow.
        for(std::vector<str>::const_iterator group = group_combo->begin(); group != group_combo->end(); group++)
        {
          matches = matches and contains_anything_interpretable_as_member_of(combo, *group);
          if (not matches) break;
        }
        //Return true immediately if all entries in the allowed group combination have been matched.
        if (matches) return true;
      }
      return false;
    }

    /// Test whether the functor has been explictly allowed to be used with a given combination of models 
    bool functor::modelComboExplicitlyAllowed(std::vector<str> combo)
    {
      // If any model in the combo is always explicitly allowed, then give the combo a thumbs up.
      for(std::vector<str>::const_iterator model = combo.begin(); model != combo.end(); model++)
      {
        if (modelExplicitlyAllowed(*model)) return true;
      }
      // Loop over the allowed combinations, and check if the passed combo matches any of them
      for(std::set<std::vector<str> >::const_iterator group_combo = allowedGroupCombos.begin(); group_combo != allowedGroupCombos.end(); group_combo++)
      {
        bool matches = true;
        //Loop over each group in the allowed group combination, and check if one of the entries in the passed model combination matches it explicitly.
        for(std::vector<str>::const_iterator group = group_combo->begin(); group != group_combo->end(); group++)
        {
          matches = matches and has_common_elements(combo, *group);
          if (not matches) break;
        }
        //Return true immediately if all entries in the allowed group combination have been matched.
        if (matches) return true;
      }
      return false;
    }

    /// Add a model group definition to the internal list of model groups.
    void functor::setModelGroup(str group, str contents)
    {
      //Strip the group contents of its parentheses, then split it, turn it into a set and save it in the map
      Utils::strip_parentheses(contents);
      std::vector<str> v = Utils::delimiterSplit(contents, ",");
      std::set<str> combo(v.begin(), v.end());
      modelGroups[group] = combo;
    }

    /// Add a model group combination to the internal list of combinations for which this functor is allowed to be used.
    void functor::setAllowedModelGroupCombo(str groups)
    {
      //Strip the group combo of its parentheses, then split it and save it in the vector of allowed combos
      Utils::strip_parentheses(groups);
      std::vector<str> group_combo = Utils::delimiterSplit(groups, ",");
      allowedGroupCombos.insert(group_combo);
    }


    /// Print function
    void functor::print(Printers::BasePrinter*)
    { 
      str warn_msg = "This is the functor base class print function! This should not\n";
      warn_msg += "be used; the print function should be redefined in daughter\n"
                  "functor classes. If this is running there is a problem somewhere.\n"
                  "Currently only functors derived from module_functor_common<!=void>\n"
                  "are allowed to try to print themselves; i.e. backend and void\n"
                  "functors may not do this (they inherit this default message).";
      utils_warning().raise(LOCAL_INFO,warn_msg);
    }    

    /// Attempt to retrieve a dependency or model parameter that has not been resolved
    void functor::failBigTime(str method)
    {
      str error_msg = "Attempted to use a functor method from a null pointer.";
      error_msg  += "\nProbably you tried to use a conditional dependency that has"
                    "\nnot been activated, or a model parameter that is not defined"    
                    "\nin the model for which this function has been invoked."   
                    "\nPlease check your module function source code."
                    "\nMethod invoked: " + method + ".";
      utils_error().raise(LOCAL_INFO,error_msg);
    }

    /// Test if there is a model in the functor's allowedModels list as which this model can be interpreted
    inline bool functor::allowed_parent_or_friend_exists(str model)
    {
      for (std::set<str>::reverse_iterator it = allowedModels.rbegin() ; it != allowedModels.rend(); ++it)
      {
        if (myClaw->model_exists(*it))
        {
          if (myClaw->downstream_of(model, *it)) return true;
        } 
      }    
      return false;    
    }

    /// Check that a model is actually part of a combination that is allowed to be used with this functor.
    inline bool functor::in_allowed_combo(str model)
    {
      // If the model is allowed on its own, just give the thumbs up immediately.
      if (modelAllowed(model)) return true;
      // Loop over the allowed combinations, and check if the passed model matches anything in any of them
      for(std::set<std::vector<str> >::const_iterator group_combo = allowedGroupCombos.begin(); group_combo != allowedGroupCombos.end(); group_combo++)
      {
        //Loop over each group in the allowed group combination, and check if the model is interpretable as a model in the group.
        for(std::vector<str>::const_iterator group = group_combo->begin(); group != group_combo->end(); group++)
        {
          // Work through the members of the model group
          std::set<str> models = modelGroups.at(*group);
          for (std::set<str>::reverse_iterator it = models.rbegin() ; it != models.rend(); ++it)
          {
            if (myClaw->model_exists(*it))
            {
              if (myClaw->downstream_of(model, *it)) return true;
            }
          }
        }
      }
      return false;
    }

    /// Test whether any of the entries in a given model group is a valid interpretation of any members in a given combination
    inline bool functor::contains_anything_interpretable_as_member_of(std::vector<str> combo, str group)
    {
      // Work through the members of the model group
      std::set<str> models = modelGroups.at(group);
      for (std::set<str>::const_iterator it = models.begin() ; it != models.end(); ++it)
      {
        if (myClaw->model_exists(*it))
        {
          // Work through the members of the combination
          for (std::vector<str>::const_iterator jt = combo.begin() ; jt != combo.end(); ++jt)
          {
            if (myClaw->model_exists(*jt))
            {
              if (myClaw->downstream_of(*jt, *it)) return true;
            }
          } 
        }    
      }
      return false;    
    }

    /// Work out whether a given combination of models and a model group have any elements in common
    inline bool functor::has_common_elements(std::vector<str> combo, str group)
    {
      // Work through the members of the model group
      std::set<str> models = modelGroups.at(group);
      for (std::set<str>::reverse_iterator it = models.rbegin() ; it != models.rend(); ++it)
      {
        if ( std::find(combo.begin(), combo.end(), *it) == combo.end() ) return true;
      }
      return false;    
    }

    /// Try to find a parent or friend model in some user-supplied map from models to sspair vectors
    str functor::find_friend_or_parent_model_in_map(str model, std::map< str, std::vector<sspair> > karta)
    {
      for (std::map< str, std::vector<sspair> >::reverse_iterator it = karta.rbegin() ; it != karta.rend(); ++it)
      {
        if (myClaw->model_exists(it->first))
        {
          if (myClaw->downstream_of(model, it->first)) return it->first;
        } 
      }    
      return "";    
    }

    // Module_functor_common class methods

    /// Constructor
    module_functor_common::module_functor_common(str func_name,
                                                 str func_capability,
                                                 str result_type,
                                                 str origin_name,
                                                 Models::ModelFunctorClaw &claw)
    : functor                  (func_name, func_capability, result_type, origin_name, claw),
      runtime_average          (FUNCTORS_RUNTIME_INIT),           // default 1 micro second
      fadeRate                 (FUNCTORS_FADE_RATE),              // can be set individually for each functor
      pInvalidation            (FUNCTORS_BASE_INVALIDATION_RATE),
      iCanManageLoops          (false),
      iRunNested               (false),
      myLoopManagerCapability  ("none"),
      myLoopManager            (NULL),
      myCurrentIteration       (NULL),
      globlMaxThreads          (omp_get_max_threads()),
      myLogTag                 (-1)
    {
      if (globlMaxThreads == 0) utils_error().raise(LOCAL_INFO,"Cannot determine number of hardware threads available on this system.");

      // Determine LogTag number
      myLogTag = Logging::str2tag(myOrigin);
      // Check for failure
      if(myLogTag==-1)
      {
        std::ostringstream ss;
        ss << "Error retrieving LogTag number (in functors.cpp, constructor for module_functor_common)! No match for module name "
           << "in tag2str map! Probably this is just a model functor, so this is no problem. (myOrigin="
           << myOrigin << ", myName=" << myName << ")";
        utils_warning().raise(LOCAL_INFO,ss.str());
      }

      if(myLogTag==-1)
      {
        logger() <<warn<<debug<<EOM;
      }
    }

    /// Destructor
    module_functor_common::~module_functor_common() 
    { 
      if (myCurrentIteration != NULL)  delete [] myCurrentIteration;
    }

    /// Getter for averaged runtime
    double module_functor_common::getRuntimeAverage()
    {
      return runtime_average;
    }

    /// Reset functor
    void module_functor_common::reset()
    {
      needs_recalculating = true;
      if (iCanManageLoops) resetLoop();
    }

    /// Tell the functor that it invalidated the current point in model space, pass a message explaining why, and throw an exception.
    void module_functor_common::notifyOfInvalidation(const str& msg)
    {
      acknowledgeInvalidation(invalid_point());
      invalid_point().raise(msg);
    }

    /// Acknowledge that this functor invalidated the current point in model space.
    void module_functor_common::acknowledgeInvalidation(invalid_point_exception& e)
    {
      pInvalidation += fadeRate*(1-FUNCTORS_BASE_INVALIDATION_RATE);
      e.set_thrower(this);
    }

    /// Getter for invalidation rate
    double module_functor_common::getInvalidationRate()
    {
      if (this == NULL) functor::failBigTime("getInvalidationRate");
      return pInvalidation;
    }

    /// Setter for the fade rate
    void module_functor_common::setFadeRate(double new_rate)
    {
      if (this == NULL) functor::failBigTime("setFadeRate");
      fadeRate = new_rate;
    }

    /// Indicate whether or not a known model is activated or not.
    bool module_functor_common::getActiveModelFlag(str model)
    {
      if (this == NULL) functor::failBigTime("getActiveModelFlag");
      if (activeModelFlags.find(model) == activeModelFlags.end())
      {
        std::ostringstream ss;
        ss << "Problem in getActiveModelFlag()." << endl
           << "Requested model " << model << " is not one of my known models." << endl
           << "I am " << myName << ", from " << myOrigin << "."; 
        utils_error().raise(LOCAL_INFO,ss.str());
      }
      return activeModelFlags.at(model);       
    }

    /// Return a safe pointer to the vector of models that this functor is currently configured to run with.
    safe_ptr< std::vector<str> > module_functor_common::getModels()
    {
      if (this == NULL) functor::failBigTime("getModels");
      return safe_ptr< std::vector<str> >(&myModels);       
    }

    /// Return a safe pointer to a string indicating which backend requirement has been activated for a given backend group.
    safe_ptr<str> module_functor_common::getChosenReqFromGroup(str group)
    {
      if (this == NULL) functor::failBigTime("getChosenReqFromGroup");
      chosenReqsFromGroups[group] = "none";
      return safe_ptr<str>(&chosenReqsFromGroups[group]);       
    }

    /// Execute a single iteration in the loop managed by this functor.
    void module_functor_common::iterate(int iteration)
    {
      if (not myNestedFunctorList.empty())
      {          
        for (std::vector<functor*>::iterator it = myNestedFunctorList.begin();
         it != myNestedFunctorList.end(); ++it) 
        {
          (*it)->setIteration(iteration);   // Tell the nested functor what iteration this is.
          (*it)->reset_and_calculate();     // Reset the nested functor so that it recalculates, then set it off
        }
      }
    } 

    // Initialise the array holding the current iteration(s) of this functor.
    void module_functor_common::init_myCurrentIteration()
    {
      int nslots = (iRunNested ? globlMaxThreads : 1); // Set the number of slots to the max number of threads allowed iff this functor can run in parallel
      myCurrentIteration = new int[nslots];            // Reserve enough space to hold as many iteration numbers as there are slots (threads) allowed
      std::fill(myCurrentIteration, myCurrentIteration+nslots, 0); // Zero them to start off
    }

    /// Tell the manager of the loop in which this functor runs that it is time to break the loop.
    void module_functor_common::breakLoopFromManagedFunctor()
    {
      if (myLoopManager == NULL)
      {
        str errmsg = "Problem whilst attempting to break out of loop:";
        errmsg +=  "\n Loop Manager not properly defined." 
                   "\n This is " + this->name() + " in " + this->origin() + ".";
        utils_error().raise(LOCAL_INFO,errmsg); //FIXME this seems to cause a crash if it is ever triggered -- why??
      }
      else
      {
        myLoopManager->breakLoop();
      }
    }

    /// Tell the functor that the loop it manages should break now.
    void module_functor_common::breakLoop() { myLoopIsDone = true; }

    /// Return a safe pointer to the flag indicating that a loop managed by this functor should break now.
    safe_ptr<bool> module_functor_common::loopIsDone() 
    {
      if (this == NULL) functor::failBigTime("loopIsDone");
      return safe_ptr<bool>(&myLoopIsDone); 
    }

    /// Provide a way to reset the flag indicating that a loop managed by this functor should break.
    void module_functor_common::resetLoop() 
    {
      myLoopIsDone = false;
    }

    /// Setter for setting the iteration number in the loop in which this functor runs
    void module_functor_common::setIteration (int iteration)
    {
      if (myCurrentIteration == NULL) init_myCurrentIteration(); // Init memory if this is the first run through. 
      myCurrentIteration[omp_get_thread_num()] = iteration;
    }

    /// Return a safe pointer to the iteration number in the loop in which this functor runs.
    omp_safe_ptr<int> module_functor_common::iterationPtr() 
    {
      if (this == NULL) functor::failBigTime("iterationPtr");
      if (myCurrentIteration == NULL) init_myCurrentIteration(); // Init memory if this is the first run through. 
      return omp_safe_ptr<int>(myCurrentIteration); 
    }

    /// Setter for specifying whether this is permitted to be a manager functor, which runs other functors nested in a loop.
    void module_functor_common::setCanBeLoopManager (bool canManage) { iCanManageLoops = canManage; }
    /// Getter for revealing whether this is permitted to be a manager functor
    bool module_functor_common::canBeLoopManager() { if (this == NULL) failBigTime("canBeLoopManager"); return iCanManageLoops; }

    /// Setter for specifying the capability required of a manager functor, if it is to run this functor nested in a loop.
    void module_functor_common::setLoopManagerCapability (str cap) { iRunNested = true; myLoopManagerCapability = cap; }
    /// Getter for revealing the required capability of the wrapped function's loop manager
    str module_functor_common::loopManagerCapability() { if (this == NULL) failBigTime("loopManagerCapability"); return myLoopManagerCapability; }
    /// Getter for revealing the name of the wrapped function's assigned loop manager
    str module_functor_common::loopManagerName() { if (this == NULL) failBigTime("loopManagerName"); return (myLoopManager == NULL ? "none" : myLoopManager->name()); }
    /// Getter for revealing the module of the wrapped function's assigned loop manager
    str module_functor_common::loopManagerOrigin() { if (this == NULL) failBigTime("loopManagerOrigin"); return (myLoopManager == NULL ? "none" : myLoopManager->origin()); }

    /// Getter for listing currently activated dependencies
    std::vector<sspair> module_functor_common::dependencies() { return myDependencies; }
    /// Getter for listing backend requirement groups
    std::vector<str> module_functor_common::backendgroups() { return myGroups; }                    
    /// Getter for listing all backend requirements
    std::vector<sspair> module_functor_common::backendreqs() { return myResolvableBackendReqs; }
    /// Getter for listing backend requirements from a specific group
    std::vector<sspair> module_functor_common::backendreqs(str group)
    { 
      if (myGroupedBackendReqs.find(group) != myGroupedBackendReqs.end())
      {
        return myGroupedBackendReqs[group]; 
      }
      else
      {
        std::vector<sspair> empty;
        return empty;
      }
    }
    /// Getter for listing permitted backends
    std::vector<sspair> module_functor_common::backendspermitted(sspair quant) 
    { 
      if (permitted_map.find(quant) != permitted_map.end())
      {
        return permitted_map[quant]; 
      }
      else
      {
        std::vector<sspair> empty;
        return empty;
      }
    }
    /// Getter for listing tags associated with backend requirements
    std::vector<str> module_functor_common::backendreq_tags(sspair quant)
    { 
      if (backendreq_tagmap.find(quant) != backendreq_tagmap.end())
      {
        return backendreq_tagmap[quant]; 
      }
      else
      {
        std::vector<str> empty;
        return empty;
      }
    }
    /// Getter for listing backend requirements that must be resolved from the same backend
    std::vector<sspair> module_functor_common::forcematchingbackend(str tag)
    { 
      if (myForcedMatches.find(tag) != myForcedMatches.end())
      {
        return myForcedMatches[tag]; 
      }
      else
      {
        std::vector<sspair> empty;
        return empty;
      }
    }

    /// Getter for listing backend-specific conditional dependencies (4-string version)
    std::vector<sspair> module_functor_common::backend_conditional_dependencies (str req, str type, str be, str ver)  
    { 
      std::vector<sspair> generic_deps, specific_deps, total_deps;
      std::vector<str> quad;
      quad.push_back(req);
      quad.push_back(type);
      quad.push_back(be);
      quad.push_back(ver);
      //Check first what conditional dependencies exist for all versions of this backend
      if (ver != "any")
      {
        generic_deps = backend_conditional_dependencies(req, type, be, "any");
      }
      //Now see if there are any for this specific version
      if (myBackendConditionalDependencies.find(quad) != myBackendConditionalDependencies.end())
      {
        specific_deps = myBackendConditionalDependencies[quad];
      }
      //Now put them together
      total_deps.reserve(generic_deps.size() + specific_deps.size());
      total_deps.insert(total_deps.end(), generic_deps.begin(), generic_deps.end());
      total_deps.insert(total_deps.end(), specific_deps.begin(), specific_deps.end());        
      return total_deps;
    }
    
    /// Getter for backend-specific conditional dependencies (3-string version)
    std::vector<sspair> module_functor_common::backend_conditional_dependencies (str req, str type, str be)  
    { 
      return backend_conditional_dependencies(req, type, be, "any");
    }
    
    /// Getter for backend-specific conditional dependencies (backend functor pointer version)
    std::vector<sspair> module_functor_common::backend_conditional_dependencies (functor* be_functor)  
    { 
      return backend_conditional_dependencies (be_functor->capability(), be_functor->type(),
       be_functor->origin(), be_functor->version());
    }

    /// Getter for listing model-specific conditional dependencies
    std::vector<sspair> module_functor_common::model_conditional_dependencies (str model)
    { 
      str parent = find_friend_or_parent_model_in_map(model,myModelConditionalDependencies);
      if (parent != "") return myModelConditionalDependencies[parent];
      std::vector<sspair> empty;
      return empty;
    }

    /// Getter for listing model-specific conditional backend requirements
    std::vector<sspair> module_functor_common::model_conditional_backend_reqs (str model)
    { 
      str parent = find_friend_or_parent_model_in_map(model,myModelConditionalBackendReqs);
      if (parent != "") return myModelConditionalBackendReqs[parent];
      std::vector<sspair> empty;
      return empty;
    }

    /// Add and activate unconditional dependencies.
    void module_functor_common::setDependency(str dep, str type, void(*resolver)(functor*, module_functor_common*), str purpose)
    {
      sspair key (dep, Utils::fix_type(type));
      myDependencies.push_back(key);
      dependency_map[key] = resolver;
      this->myPurpose = purpose; // only relevant for output nodes
    }

    /// Add a backend conditional dependency for multiple backend versions
    void module_functor_common::setBackendConditionalDependency
     (str req, str be, str ver, str dep, str dep_type, void(*resolver)(functor*, module_functor_common*))
    {
      // Split the version string and send each version to be registered
      std::vector<str> versions = Utils::delimiterSplit(ver, ",");
      for (std::vector<str>::iterator it = versions.begin() ; it != versions.end(); ++it)
      {
        setBackendConditionalDependencySingular(req, be, *it, dep, dep_type, resolver);
      }
    }

    /// Add a backend conditional dependency for a single backend version
    void module_functor_common::setBackendConditionalDependencySingular
     (str req, str be, str ver, str dep, str dep_type, void(*resolver)(functor*, module_functor_common*))
    {
      sspair key (dep, Utils::fix_type(dep_type));
      std::vector<str> quad;
      if (backendreq_types.find(req) != backendreq_types.end())
      {
        quad.push_back(req);
        quad.push_back(backendreq_types[req]);
        quad.push_back(be);
        quad.push_back(ver);
      }
      else
      {
        str errmsg = "Problem whilst attempting to set backend-conditional dependency:";
        errmsg +=  "\nThe type of the backend requirement " + req + "on which the" 
                   "\ndependency " + dep + " is conditional has not been set.  This"
                   "\nis " + this->name() + " in " + this->origin() + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }
      if (myBackendConditionalDependencies.find(quad) == myBackendConditionalDependencies.end())
      {
        std::vector<sspair> newvec;
        myBackendConditionalDependencies[quad] = newvec;
      }
      myBackendConditionalDependencies[quad].push_back(key);
      dependency_map[key] = resolver;
    }

    /// Add a model conditional dependency for multiple models
    void module_functor_common::setModelConditionalDependency
     (str model, str dep, str dep_type, void(*resolver)(functor*, module_functor_common*))
    {
      // Split the model string and send each model to be registered
      std::vector<str> models = Utils::delimiterSplit(model, ",");
      for (std::vector<str>::iterator it = models.begin() ; it != models.end(); ++it)
      {
        setModelConditionalDependencySingular(*it, dep, dep_type, resolver);
      }
    }

    /// Add a model conditional dependency for a single model
    void module_functor_common::setModelConditionalDependencySingular
     (str model, str dep, str dep_type, void(*resolver)(functor*, module_functor_common*))
    { 
      sspair key (dep, Utils::fix_type(dep_type));
      if (myModelConditionalDependencies.find(model) == myModelConditionalDependencies.end())
      {
        std::vector<sspair> newvec;
        myModelConditionalDependencies[model] = newvec;
      }
      myModelConditionalDependencies[model].push_back(key);
      dependency_map[key] = resolver;
    }

    /// Add an unconditional backend requirement
    /// The info gets updated later if this turns out to be conditional on a model. 
    void module_functor_common::setBackendReq(str group, str req, str tags, str type, void(*resolver)(functor*))
    { 
      type = Utils::fix_type(type);
      sspair key (req, type);
      backendreq_types[req] = type;
      myBackendReqs.push_back(key);
      myResolvableBackendReqs.push_back(key);
      if ( std::find(myGroups.begin(), myGroups.end(), group) == myGroups.end() )
      {
        myGroups.push_back(group);
        std::vector<sspair> empty;
        myGroupedBackendReqs[group] = empty;
      }
      myGroupedBackendReqs[group].push_back(key);
      backendreq_map[key] = resolver;
      Utils::strip_parentheses(tags);
      backendreq_tagmap[key] = Utils::delimiterSplit(tags, ",");      
      backendreq_groups[key] = group;
    }

    /// Add a rule for activating backend requirements according to the model being scanned.
    void module_functor_common::makeBackendRuleForModel(str model, str tag)
    {
      //Strip the tag and model strings of their parentheses
      Utils::strip_parentheses(tag);
      Utils::strip_parentheses(model);

      //Split the tag string and sort it.
      std::vector<str> tags = Utils::delimiterSplit(tag, ",");
      std::sort(tags.begin(), tags.end());

      //Find all declared backend requirements that fit one of the tags within the passed tag set.
      for (std::vector<sspair>::iterator it = myBackendReqs.begin(); it != myBackendReqs.end(); ++it)
      {
        std::vector<str> tagset = backendreq_tagmap[*it];
        std::sort(tagset.begin(), tagset.end());
        if (not Utils::is_disjoint(tags, tagset))
        {
          // Make each of the matching backend requirements conditional on the models passed in.
          setModelConditionalBackendReq(model,it->first,it->second);
        }
      }

    }

    /// Add a model conditional backend requirement for multiple models
    void module_functor_common::setModelConditionalBackendReq
     (str model, str req, str type)
    {
      // Split the model string and send each model to be registered
      std::vector<str> models = Utils::delimiterSplit(model, ",");
      for (std::vector<str>::iterator it = models.begin() ; it != models.end(); ++it)
      {
        setModelConditionalBackendReqSingular(*it, req, type);
      }
    }

    /// Add a model conditional backend requirement for a single model
    void module_functor_common::setModelConditionalBackendReqSingular
     (str model, str req, str type)
    { 
      sspair key (req, Utils::fix_type(type));

      // Remove the entry from the resolvable backend reqs list...
      myResolvableBackendReqs.erase(std::remove(myResolvableBackendReqs.begin(), 
       myResolvableBackendReqs.end(), key), myResolvableBackendReqs.end());

      // Remove the entry from the resolvable backend reqs list of the group this req is in...
      std::vector<sspair>* resolvableGroupBackendReqs = &(myGroupedBackendReqs[backendreq_groups[key]]);
      resolvableGroupBackendReqs->erase(std::remove(resolvableGroupBackendReqs->begin(), 
       resolvableGroupBackendReqs->end(), key), resolvableGroupBackendReqs->end());

      // Check that the model is not already in the conditional backend reqs list, then add it
      if (myModelConditionalBackendReqs.find(model) == myModelConditionalBackendReqs.end())
      {
        std::vector<sspair> newvec;
        myModelConditionalBackendReqs[model] = newvec;
      }
      myModelConditionalBackendReqs[model].push_back(key);
    }

    /// Add a rule for dictating which backends can be used to fulfill which backend requirements.
    void module_functor_common::makeBackendOptionRule(str be_and_ver, str tag)
    {
      //Strip the tag and be-ver strings of their parentheses, then split them
      Utils::strip_parentheses(tag);
      Utils::strip_parentheses(be_and_ver);
      std::vector<str> tags = Utils::delimiterSplit(tag, ",");
      std::vector<str> be_plus_versions = Utils::delimiterSplit(be_and_ver, ",");

      //Die if no backend and/or no tags were given.      
      if (tags.empty() or be_plus_versions.empty())  
      {
        str errmsg = "Error whilst attempting to set permitted backends:";
        errmsg +=  "\nA BACKEND_OPTION must include a backend name and at least one tag."
                   "\nThis is " + this->name() + " in " + this->origin() + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }
     
      //Sort the tags vector.
      std::sort(tags.begin(), tags.end());

      //Seperate the backend from the versions
      str be = be_plus_versions.at(0);
      std::vector<str> versions(be_plus_versions.begin()+1,be_plus_versions.end());
      if (versions.empty()) versions.push_back("any");

      //Find all declared backend requirements that fit one of the tags within the passed tag set.
      for (std::vector<sspair>::iterator it = myBackendReqs.begin(); it != myBackendReqs.end(); ++it)
      {
        std::vector<str> tagset = backendreq_tagmap[*it];
        std::sort(tagset.begin(), tagset.end());
        if (not Utils::is_disjoint(tags, tagset))
        {
          // For each of the matching backend requirements, set the chosen backend-version pairs as permitted 
          for (std::vector<str>::iterator vit = versions.begin() ; vit != versions.end(); ++vit)
          {
            setPermittedBackend(it->first, be, *vit);
          }
        }
      }

    }

    /// Add a single permitted backend version
    void module_functor_common::setPermittedBackend(str req, str be, str ver)
    { 
      sspair key;
      if (backendreq_types.find(req) != backendreq_types.end())
      {
        key = std::make_pair(req, backendreq_types[req]);
      }
      else
      {
        str errmsg = "Error whilst attempting to set permitted backend:";
        errmsg += "\nThe return type of the backend requirement " + req + "is not set." 
                  "\nThis probably means the backend requirement has not been declared."
                  "\nThis is " + this->name() + " in " + this->origin() + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }
      sspair vector_entry (be,  ver);
      if (permitted_map.find(key) == permitted_map.end())
      {
        std::vector<sspair> newvec;
        permitted_map[key] = newvec;
      }
      permitted_map[key].push_back(vector_entry);       
    }

    /// Add one or more rules that force backends reqs with the same tag to always be resolved from the same backend.
    void module_functor_common::makeBackendMatchingRule(str tag)
    {
      //Strip the tag string of any parentheses, then split it
      Utils::strip_parentheses(tag);
      std::vector<str> tags = Utils::delimiterSplit(tag, ",");

      //Die if no tags were given.      
      if (tags.empty())  
      {
        str errmsg = "Problem whilst attempting to set backend-matching rule:";
        errmsg +=  "\nA call to FORCE_SAME_BACKEND must include at least one tag!"
                   "\nThis is " + this->name() + " in " + this->origin() + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }

      //Work with one tag at a time
      for (std::vector<str>::iterator tagit = tags.begin(); tagit != tags.end(); ++tagit)
      {
        std::vector<sspair> matches;
        //Find all declared backend requirements that fit the current tag
        for (std::vector<sspair>::iterator it = myBackendReqs.begin(); it != myBackendReqs.end(); ++it)
        {
          //Test if the current tag is amongst the tags listed for the current backend requirement
          std::vector<str> its_tags = backendreq_tagmap[*it];
          if ( std::find(its_tags.begin(), its_tags.end(), *tagit) != its_tags.end() )
          {
            //It is; now place the current backend requirement into the set of matches
            matches.push_back(*it);
          }
        }
        //Save the matched set of backend requirements as needing to be filled using the same backend.
        myForcedMatches[*tagit] = matches;
      }

      //Note that overlapping sets are explicitly left unmerged, as the common elements 
      //may or may not be activated in a given scan, making the disjointedness of the 
      //different sets determinable only at resolution time, not initialisation time.

    }

    /// Add a rule indicating that classes from a given backend must be available
    void module_functor_common::setRequiredClassloader(str be, str ver) { required_classloading_backends[be].insert(ver); }

    /// Indicate to the functor which backends are actually loaded and working
    void module_functor_common::notifyOfBackends(std::map<str, std::set<str> > be_ver_map)
    {
      // Loop over all the backends that are needed for this functor to work.
      for (auto it = required_classloading_backends.begin(); it != required_classloading_backends.end(); ++it)
      {
        // Check to make sure some version of the backend in question is connected.
        if (be_ver_map.find(it->first) == be_ver_map.end())
        {
          this->myStatus = -3;
        }
        else 
        {  // Loop over all the versions of the backend that are needed for this functor to work.
          for (auto jt = it->second.begin(); jt != it->second.end(); ++jt)
          {
            std::set<str> versions = be_ver_map.at(it->first);
            // Check that the specific version needed is connected.
            if (versions.find(*jt) == versions.end()) this->myStatus = -3;
          }
        }
      }
    } 

    /// Set the ordered list of pointers to other functors that should run nested in a loop managed by this one
    void module_functor_common::setNestedList (std::vector<functor*> &newNestedList)
    { 
      if (iCanManageLoops) 
      {
        myNestedFunctorList = newNestedList;
      }
      else
      {
        str errmsg = "This module functor is not permitted to manage";
        errmsg +=  "\nloops, so you cannot set its nested functor list."
                   "\nThis is " + this->name() + " in " + this->origin() + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }
    } 

    /// Resolve a dependency using a pointer to another functor object
    void module_functor_common::resolveDependency (functor* dep_functor)
    {
      sspair key (dep_functor->quantity());
      if (std::find(myDependencies.begin(), myDependencies.end(), key) == myDependencies.end())                            
      {                                                                      
        str errmsg = "Cannot resolve dependency:";
        errmsg +=  "\nFunction " + myName + " in " + myOrigin + " does not depend on"
                   "\ncapability " + key.first + " with type = " + key.second + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }
      else
      {
        if (dependency_map.find(key) != dependency_map.end()) (*dependency_map[key])(dep_functor,this);
        // propagate purpose from next to next-to-output nodes
        dep_functor->setPurpose(this->myPurpose);
      }
    }

    // Set this functor's loop manager (if it has one)
    void module_functor_common::resolveLoopManager (functor* dep_functor)
    { 
      if (dep_functor->capability() != myLoopManagerCapability or not dep_functor->canBeLoopManager()) 
      {                                                                      
        sspair key (dep_functor->quantity());
        str errmsg = "Cannot set loop manager for nested functor:";
        errmsg +=  "\nFunction " + myName + " in " + myOrigin + " does not need a loop manager with"
                   "\ncapability " + key.first + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }
      myLoopManager = dep_functor;
    }

    /// Resolve a backend requirement using a pointer to another functor object
    void module_functor_common::resolveBackendReq (functor* be_functor)
    {

      sspair key (be_functor->quantity());

      //First make sure that the proposed backend function fulfills a known requirement of the module function.
      if (backendreq_map.find(key) != backendreq_map.end())                            
      { 

        sspair proposal (be_functor->origin(), be_functor->version());  //Proposed backend-version pair
        sspair generic_proposal (be_functor->origin(), "any");          //Proposed backend, any version 

        if ( //Check for a condition under which the proposed backend function is an acceptable fit for this requirement.
         //Check whether there are have been any permitted backends stated for this requirement (if not, anything goes).
         (permitted_map.find(key) == permitted_map.end()) || 
         //Iterate over the vector of backend-version pairs for this requirement to see if all versions of the 
         //proposed backend have been explicitly permitted.
         (std::find(permitted_map[key].begin(), permitted_map[key].end(), generic_proposal) != permitted_map[key].end()) ||
         //Iterate over the vector of backend-version pairs again to see if the specific backend version 
         //proposed had been explicitly permitted.
         (std::find(permitted_map[key].begin(), permitted_map[key].end(), proposal) != permitted_map[key].end()) )         
        {

          //One of the conditions was met, so do the resolution.
          (*backendreq_map[key])(be_functor);

          //Set this backend functor's status to active.
          be_functor->setStatus(2);
       
          //If this is also the condition under which any backend-conditional dependencies should be activated, do it.
          std::vector<sspair> deps_to_activate = backend_conditional_dependencies(be_functor);
          for (std::vector<sspair>::iterator it = deps_to_activate.begin() ; it != deps_to_activate.end(); ++it)
          {
            myDependencies.push_back(*it);        
          }
   
          // Add a dependency on the initialisation function of the backend that this backend function hails from (if not done already).          
          sspair be_ini_quantity(be_functor->origin() + "_" + be_functor->safe_version() + "_init", "void");
          if (std::find(myDependencies.begin(), myDependencies.end(), be_ini_quantity) == myDependencies.end()) 
          {
            myDependencies.push_back(be_ini_quantity);
          }

          //Check if this backend requirement is part of a group.
          str group = backendreq_groups[key];
          if (group != "none")           
          {                                                      
            //If it is part of a group, make sure that group has actually been declared.
            if (chosenReqsFromGroups.find(group) != chosenReqsFromGroups.end() )
            {
              //If it is part of a group, make sure another backend requirement from the same group has not already been activated.
              if (chosenReqsFromGroups[group] == "none")          
              {
                //If not, then this this specific backend requirement is now the active one within its group.
                chosenReqsFromGroups[group] = key.first; 
              }
              else //This backend requirement is not the first from its group to be resolved.
              {
                str errmsg = "Cannot resolve backend requirement in group " + group + ":";
                errmsg +=  "\nFunction " + myName + " in " + myOrigin + " has already had backend " 
                           "\nrequirement " + chosenReqsFromGroups[group] + " from the same group filled.";
                utils_error().raise(LOCAL_INFO,errmsg);
              }
            }
            else //This backend requirement is part of group that was not declared.
            {
              str errmsg = "Cannot resolve backend requirement in group " + group + ":";
              errmsg +=  "\nThis group has not been declared.  Please add"
                         "\n BACKEND_GROUP("+group+")"
                         "\nTo the rollcall declaration of " + myName + " in " + myOrigin + ".";
              utils_error().raise(LOCAL_INFO,errmsg);
            }
          }

        }

        else //The proposed backend function is not an acceptable fit for this requirement.
        { 
          str errmsg = "Cannot resolve backend requirement:";
          errmsg +=  "\nBackend capability " + key.first + " with type " + key.second +
                     "\nrequired by function " + myName + " in " + myOrigin + " is not permitted"
                     "\nto use " + proposal.first + ", version " + proposal.second + ".";
          utils_error().raise(LOCAL_INFO,errmsg);
        } 

      }

      else //The proposed backend function does not fulfill any known requirement of the module function.
      {                                                                      
        str errmsg = "Cannot resolve backend requirement:";
        errmsg +=  "\nFunction " + myName + " in " + myOrigin + " does not require"
                   "\nbackend capability " + key.first + " with type " + key.second + ".";
        utils_error().raise(LOCAL_INFO,errmsg);
      }        

    }

    /// Notify the functor that a certain model is being scanned, so that it can activate its dependencies and backend reqs accordingly.
    void module_functor_common::notifyOfModel(str model)
    {
      // Add the model to the internal list of models being scanned.
      myModels.push_back(model);

      // Construct the list of known models. First get all the explicitly allowed models.
      for (auto it = allowedModels.begin(); it != allowedModels.end(); ++it) { activeModelFlags[*it] = false; }
      // Next get all the models in allowed groups
      for (auto it = allowedGroupCombos.begin(); it != allowedGroupCombos.end(); ++it)
      { for (auto jt = it->begin(); jt != it->end(); ++jt) { activeModelFlags[*jt] = false; } }
      // Next get all the models mentioned in conditional dependencies and backend reqs
      for (auto it = myModelConditionalDependencies.begin(); it != myModelConditionalDependencies.end(); ++it) { activeModelFlags[it->first] = false; }
      for (auto it = myModelConditionalBackendReqs.begin();  it != myModelConditionalBackendReqs.end();  ++it) { activeModelFlags[it->first] = false; }

      // Now activate the flags for the models that are being used. TODO make this follow fulfillment of modelparameter deps (how?)

      // If this model fits any conditional dependencies (or descended from one that can be interpreted as one that fits any), then activate them.
      std::vector<sspair> deps_to_activate = model_conditional_dependencies(model);          
      for (std::vector<sspair>::iterator it = deps_to_activate.begin() ; it != deps_to_activate.end(); ++it)
      {
        myDependencies.push_back(*it);        
      }
      // If this model fits any conditional backend requirements (or descended from one that can be interpreted as one that fits any), then activate them.
      std::vector<sspair> backend_reqs_to_activate = model_conditional_backend_reqs(model);
      if (verbose) cout << "model: " << model << endl;
      for (std::vector<sspair>::iterator it = backend_reqs_to_activate.begin() ; it != backend_reqs_to_activate.end(); ++it)
      {
        if (verbose) cout << "req: " << it->first << " " << it->second << endl;
        myResolvableBackendReqs.push_back(*it);
        myGroupedBackendReqs[backendreq_groups[*it]].push_back(*it);
      }
    }

    /// Do pre-calculate timing things
    void module_functor_common::startTiming(double & nsec, double & sec)
    {
      (void)nsec; (void)sec;
      start = std::chrono::system_clock::now();
    }

    /// Do post-calculate timing things
    void module_functor_common::finishTiming(double nsec, double sec)
    {
      (void)nsec; (void) sec;
      end = std::chrono::system_clock::now();
      runtime = end-start;
      runtime_average = runtime_average*(1-fadeRate) + fadeRate*runtime.count();
      pInvalidation = pInvalidation*(1-fadeRate) + fadeRate*FUNCTORS_BASE_INVALIDATION_RATE;
      if (not omp_in_parallel()) cout << "Runtime " << myName << ": " << runtime.count() << " s (" << runtime_average << " s)" << endl;
      needs_recalculating = false;
    }


    /// Class methods for actual module functors for TYPE != void

    template <typename TYPE>
    module_functor<TYPE>::module_functor(void (*inputFunction)(TYPE &),
                                   str func_name,
                                   str func_capability,
                                   str result_type,
                                   str origin_name,
                                   Models::ModelFunctorClaw &claw)
    : module_functor_common(func_name, func_capability, result_type, origin_name, claw),
      myFunction  (inputFunction),
      myValue     (NULL),
      myPrintFlag (false)
    {}

    /// Destructor
    template <typename TYPE>
    module_functor<TYPE>::~module_functor() 
    { 
      if (myValue != NULL) delete [] myValue;
    }

    /// Setter for indicating if the wrapped function's result should be printed
    template <typename TYPE>
    void module_functor<TYPE>::setPrintRequirement(bool flag) { if (this == NULL) failBigTime("setPrintRequirement"); myPrintFlag = flag;}

    /// Getter indicating if the wrapped function's result should be printed
    template <typename TYPE>
    bool module_functor<TYPE>::requiresPrinting() const { if (this == NULL) failBigTime("requiresPrinting"); return myPrintFlag; }

    /// Calculate method
    template <typename TYPE>
    void module_functor<TYPE>::calculate()
    {
      if (myValue == NULL) init_myValue(); // Init memory if this is the first run through. 
      if (needs_recalculating)             // Do the actual calculation if required.

      {
        logger().entering_module(myLogTag);
        double nsec = 0, sec = 0;
        this->startTiming(nsec,sec);                       //Begin timing function evaluation
        try 
        {
          this->myFunction(myValue[omp_get_thread_num()]); //Run and place result in the appropriate slot in myValue
        }
        catch (invalid_point_exception& e)
        {
          acknowledgeInvalidation(e);
          throw(e);
        }
        this->finishTiming(nsec,sec);                      //Stop timing function evaluation
        logger().leaving_module();
      }

    }

    // Initialise the memory of this functor.
    template <typename TYPE>
    void module_functor<TYPE>::init_myValue()
    {
      // Reserve enough space to hold as many results as there are slots (threads) allowed
      myValue = new TYPE[(iRunNested ? globlMaxThreads : 1)];                      
    }

    /// Operation (return value)
    template <typename TYPE>
    TYPE module_functor<TYPE>::operator()(int index) 
    { 
      if (this == NULL) functor::failBigTime("operator()");
      if (myValue == NULL) init_myValue(); // Init memory if this is the first run through. 
      return (iRunNested ? myValue[index] : myValue[0]);
    }

    /// Alternative to operation (returns a safe pointer to value)
    template <typename TYPE>
    safe_ptr<TYPE> module_functor<TYPE>::valuePtr()
    {
      if (this == NULL) functor::failBigTime("valuePtr");
      if (myValue == NULL) init_myValue(); // Init memory if this is the first run through. 
      return safe_ptr<TYPE>(myValue);
    }

    /// Printer function
    template <typename TYPE>
    void module_functor<TYPE>::print(Printers::BasePrinter* printer, int index)
    {
      if(myPrintFlag)
      {
        if (myValue == NULL) init_myValue(); // Init memory if this is the first run through. 
        if (not iRunNested) index = 0; // Force printing of index=0 if this functor cannot run nested. 
        printer->print(myValue[index],myLabel,myVertexID);
      }
    }

    /// Printer function (no-thread-index short-circuit)
    template <typename TYPE>
    void module_functor<TYPE>::print(Printers::BasePrinter* printer) { print(printer,0); }


  /// Class methods for actual module functors for TYPE=void

    /// Constructor
    module_functor<void>::module_functor(void (*inputFunction)(),
                                         str func_name,
                                         str func_capability,
                                         str result_type,
                                         str origin_name,
                                         Models::ModelFunctorClaw &claw)
    : module_functor_common(func_name, func_capability, result_type, origin_name, claw),
      myFunction (inputFunction) {}

    /// Calculate method
    void module_functor<void>::calculate()
    {
      if (needs_recalculating)
      {
        logger().entering_module(myLogTag);
        double nsec = 0, sec = 0;
        this->startTiming(nsec,sec);
        try
        {
          this->myFunction();
        }
        catch (invalid_point_exception& e)
        {
          acknowledgeInvalidation(e);
          throw(e);
        }
        this->finishTiming(nsec,sec);
        logger().leaving_module();
      }
    }

    /// Blank print method
    void module_functor<void>::print(Printers::BasePrinter*, int) {} 


  /// Model functor class method definitions
    
    /// Constructor
    model_functor::model_functor(void (*inputFunction)(ModelParameters &),
                                 str func_name,
                                 str func_capability,
                                 str result_type,
                                 str origin_name,
                                 Models::ModelFunctorClaw &claw)
    : module_functor<ModelParameters>(inputFunction, func_name, func_capability, result_type, origin_name, claw)
    {
      init_myValue();
    }   
    
    /// Function for adding a new parameter to the map inside the ModelParameters object
    void model_functor::addParameter(str parname)
    {
      myValue->_definePar(parname);
    }

    /// Function for handing over parameter identities to another model_functor
    void model_functor::donateParameters(model_functor &receiver)
    {
      for(std::map<std::string,double>::const_iterator it = myValue->getValuesPtr()->begin();
          it != myValue->getValuesPtr()->end(); 
          it++)
      {
        receiver.addParameter(it->first);
      }
    }


  /// Primary model functor class method definitions
   
    /// Constructor
    primary_model_functor::primary_model_functor(void (*inputFunction)(ModelParameters &),
                                                 str func_name,
                                                 str func_capability,
                                                 str result_type,
                                                 str origin_name,
                                                 Models::ModelFunctorClaw &claw)
    : model_functor(inputFunction, func_name, func_capability, result_type, origin_name, claw) {}   
    
    /// Functor contents raw pointer "get" function
    /// Returns a raw pointer to myValue, so that the contents may be 
    /// modified (intended for setting parameter values in primary 
    /// ModelParameters objects)
    ModelParameters* primary_model_functor::getcontentsPtr()
    {
      if (this == NULL) functor::failBigTime("getcontentsPtr");
      return myValue;
    }

#define INSTANTIATE_MODULE_FUNCTOR_TEMPLATE(r,x,TYPE)  template class module_functor<TYPE>; 
INSTANTIATE_MODULE_FUNCTOR_TEMPLATE(,,void)
BOOST_PP_SEQ_FOR_EACH(INSTANTIATE_MODULE_FUNCTOR_TEMPLATE,,PRINTABLE_TYPES)

//Maybe one day... 
//#define INSTANTIATE_BACKEND_FUNCTOR_TEMPLATE(...)      template class backend_functor<__VA_ARGS__>;
//BOOST_PP_SEQ_FOR_EACH(INSTANTIATE_BACKEND_FUNCTOR_TEMPLATE,,BACKEND_TYPES)

}
