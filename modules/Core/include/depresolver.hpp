//   GAMBIT: Global and Modular BSM Inference Tool
//   *********************************************
///  \file
///
///  Dependency resolution with boost graph library
///
///  *********************************************
///
///  Authors (add name and date if you modify):
///   
///  \author Christoph Weniger
///          (c.weniger@uva.nl)
///  \date 2013 Apr, May, Jun, Jul
///
///  \author Pat Scott 
///          (patscott@physics.mcgill.ca)
///  \date 2013 May, Aug, Nov
///
///  *********************************************

#ifndef __depresolver_hpp__
#define __depresolver_hpp__

#include <string>
#include <list>
#include <vector>
#include <map>
#include <queue>

#include "gambit_core.hpp"
#include "printers_rollcall.hpp"
#include "functors.hpp"
#include "error_handlers.hpp"
#include "yaml_parser.hpp"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>

namespace Gambit
{

  namespace DRes
  {

    using namespace boost;

    // Typedefs for central boost graph
    typedef adjacency_list<vecS, vecS, bidirectionalS, functor*, vecS> MasterGraphType;
    typedef graph_traits<MasterGraphType>::vertex_descriptor VertexID;
    typedef graph_traits<MasterGraphType>::edge_descriptor EdgeID;
    typedef property_map<MasterGraphType,vertex_index_t>::type IndexMap;

    // Typedefs for communication channels with the master-likelihood
    typedef std::map<std::string, double *> inputMapType;
    typedef std::map<std::string, std::vector<functor*> > outputMapType;

    // Minimal info about outputVertices
    struct OutputVertexInfo
    {
      VertexID vertex;
      const IniParser::ObservableType * iniEntry;
    };

    // Information in parameter queue
    struct QueueEntry
    {
      QueueEntry() {}
      QueueEntry(sspair a, DRes::VertexID b, int c, bool d)
      {
        first = a;
        second = b;
        third = c;
        printme = d;
      }
      sspair first;
      DRes::VertexID second;
      int third;
      bool printme;
    };

    // Check whether s1 (wildcard + regex allowed) matches s2
    bool stringComp(str s1, str s2);

    // Main dependency resolver
    class DependencyResolver
    {
      public:
        // Constructor, provide module and backend functor lists
        DependencyResolver(const gambit_core&, const IniParser::IniFile&, Printers::BasePrinter&);

        // The dependency resolution
        void doResolution();

        // Pretty print module functor information
        void printFunctorList();

        // Pretty print function evaluation order
        void printFunctorEvalOrder();

        // New IO routines
        std::vector<VertexID> getObsLikeOrder();

        void calcObsLike(VertexID);

        double getObsLike(VertexID);

        const IniParser::ObservableType * getIniEntry(VertexID);

        void notifyOfInvalidation(VertexID);

        void resetAll();

      private:
        // Adds list of functor pointers to master graph
        void addFunctors();

        // Pretty print backend functor information
        str printGenericFunctorList(const std::vector<functor*>&);

        // Initialise the printer object with a list of functors for it to expect to be printed.
        void initialisePrinter();

        /// Deactivate functors that are not allowed to be used with the model(s) being scanned. 
        void makeFunctorsModelCompatible();

        // Resolution of individual module function dependencies
        boost::tuple<const IniParser::ObservableType *, DRes::VertexID>
          resolveDependency(DRes::VertexID toVertex, sspair quantity);

        // Generate full dependency tree
        void generateTree(std::queue<QueueEntry> parQueue);

        // Helper functions/arrays
        void fillParQueue(std::queue<QueueEntry> *parQueue,
            DRes::VertexID vertex);

        // Topological sort
        std::list<VertexID> run_topological_sort();

        // Find entries (comparison of inifile entry with quantity or functor)
        const IniParser::ObservableType * findIniEntry(
            sspair quantity, const IniParser::ObservablesType &, const str &);
        const IniParser::ObservableType * findIniEntry(
            DRes::VertexID toVertex, const IniParser::ObservablesType &, const str &);

        // Resolution of backend dependencies
        void resolveVertexBackend(VertexID);

        //
        // Private data members
        //

        // Core to which this dependency resolver is bound
        const gambit_core *boundCore;

        // ini file to which this dependency resolver is bound
        const IniParser::IniFile *boundIniFile;

        // Printer object to which this dependency resolver is bound
        Printers::BasePrinter *boundPrinter;

        // *** Output Vertex Infos
        std::vector<OutputVertexInfo> outputVertexInfos;

        // *** The central boost graph object
        MasterGraphType masterGraph;

        // *** Saved calling order for functions
        std::list<VertexID> function_order;

        // Temporary map for loop manager -> list of nested functions
        std::map<VertexID, std::set<VertexID>> loopManagerMap;

        // Indices associated with graph vertices (used by printers to identify functors)
        IndexMap index;

    };
  }
}
#endif /* defined(__depresolver_hpp__) */
