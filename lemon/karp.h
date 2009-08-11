/* -*- C++ -*-
 *
 * This file is a part of LEMON, a generic C++ optimization library
 *
 * Copyright (C) 2003-2008
 * Egervary Jeno Kombinatorikus Optimalizalasi Kutatocsoport
 * (Egervary Research Group on Combinatorial Optimization, EGRES).
 *
 * Permission to use, modify and distribute this software is granted
 * provided that this copyright notice appears in all copies. For
 * precise terms see the accompanying LICENSE file.
 *
 * This software is provided "AS IS" with no warranty of any kind,
 * express or implied, and with no claim as to its suitability for any
 * purpose.
 *
 */

#ifndef LEMON_KARP_H
#define LEMON_KARP_H

/// \ingroup shortest_path
///
/// \file
/// \brief Karp's algorithm for finding a minimum mean cycle.

#include <vector>
#include <limits>
#include <lemon/core.h>
#include <lemon/path.h>
#include <lemon/tolerance.h>
#include <lemon/connectivity.h>

namespace lemon {

  /// \brief Default traits class of Karp algorithm.
  ///
  /// Default traits class of Karp algorithm.
  /// \tparam GR The type of the digraph.
  /// \tparam LEN The type of the length map.
  /// It must conform to the \ref concepts::ReadMap "ReadMap" concept.
#ifdef DOXYGEN
  template <typename GR, typename LEN>
#else
  template <typename GR, typename LEN,
    bool integer = std::numeric_limits<typename LEN::Value>::is_integer>
#endif
  struct KarpDefaultTraits
  {
    /// The type of the digraph
    typedef GR Digraph;
    /// The type of the length map
    typedef LEN LengthMap;
    /// The type of the arc lengths
    typedef typename LengthMap::Value Value;

    /// \brief The large value type used for internal computations
    ///
    /// The large value type used for internal computations.
    /// It is \c long \c long if the \c Value type is integer,
    /// otherwise it is \c double.
    /// \c Value must be convertible to \c LargeValue.
    typedef double LargeValue;

    /// The tolerance type used for internal computations
    typedef lemon::Tolerance<LargeValue> Tolerance;

    /// \brief The path type of the found cycles
    ///
    /// The path type of the found cycles.
    /// It must conform to the \ref lemon::concepts::Path "Path" concept
    /// and it must have an \c addBack() function.
    typedef lemon::Path<Digraph> Path;
  };

  // Default traits class for integer value types
  template <typename GR, typename LEN>
  struct KarpDefaultTraits<GR, LEN, true>
  {
    typedef GR Digraph;
    typedef LEN LengthMap;
    typedef typename LengthMap::Value Value;
#ifdef LEMON_HAVE_LONG_LONG
    typedef long long LargeValue;
#else
    typedef long LargeValue;
#endif
    typedef lemon::Tolerance<LargeValue> Tolerance;
    typedef lemon::Path<Digraph> Path;
  };


  /// \addtogroup shortest_path
  /// @{

  /// \brief Implementation of Karp's algorithm for finding a minimum
  /// mean cycle.
  ///
  /// This class implements Karp's algorithm for finding a directed
  /// cycle of minimum mean length (cost) in a digraph.
  ///
  /// \tparam GR The type of the digraph the algorithm runs on.
  /// \tparam LEN The type of the length map. The default
  /// map type is \ref concepts::Digraph::ArcMap "GR::ArcMap<int>".
#ifdef DOXYGEN
  template <typename GR, typename LEN, typename TR>
#else
  template < typename GR,
             typename LEN = typename GR::template ArcMap<int>,
             typename TR = KarpDefaultTraits<GR, LEN> >
#endif
  class Karp
  {
  public:

    /// The type of the digraph
    typedef typename TR::Digraph Digraph;
    /// The type of the length map
    typedef typename TR::LengthMap LengthMap;
    /// The type of the arc lengths
    typedef typename TR::Value Value;

    /// \brief The large value type
    ///
    /// The large value type used for internal computations.
    /// Using the \ref KarpDefaultTraits "default traits class",
    /// it is \c long \c long if the \c Value type is integer,
    /// otherwise it is \c double.
    typedef typename TR::LargeValue LargeValue;

    /// The tolerance type
    typedef typename TR::Tolerance Tolerance;

    /// \brief The path type of the found cycles
    ///
    /// The path type of the found cycles.
    /// Using the \ref KarpDefaultTraits "default traits class",
    /// it is \ref lemon::Path "Path<Digraph>".
    typedef typename TR::Path Path;

    /// The \ref KarpDefaultTraits "traits class" of the algorithm
    typedef TR Traits;

  private:

    TEMPLATE_DIGRAPH_TYPEDEFS(Digraph);

    // Data sturcture for path data
    struct PathData
    {
      LargeValue dist;
      Arc pred;
      PathData(LargeValue d, Arc p = INVALID) :
        dist(d), pred(p) {}
    };

    typedef typename Digraph::template NodeMap<std::vector<PathData> >
      PathDataNodeMap;

  private:

    // The digraph the algorithm runs on
    const Digraph &_gr;
    // The length of the arcs
    const LengthMap &_length;

    // Data for storing the strongly connected components
    int _comp_num;
    typename Digraph::template NodeMap<int> _comp;
    std::vector<std::vector<Node> > _comp_nodes;
    std::vector<Node>* _nodes;
    typename Digraph::template NodeMap<std::vector<Arc> > _out_arcs;

    // Data for the found cycle
    LargeValue _cycle_length;
    int _cycle_size;
    Node _cycle_node;

    Path *_cycle_path;
    bool _local_path;

    // Node map for storing path data
    PathDataNodeMap _data;
    // The processed nodes in the last round
    std::vector<Node> _process;

    Tolerance _tolerance;
    
    // Infinite constant
    const LargeValue INF;

  public:

    /// \name Named Template Parameters
    /// @{

    template <typename T>
    struct SetLargeValueTraits : public Traits {
      typedef T LargeValue;
      typedef lemon::Tolerance<T> Tolerance;
    };

    /// \brief \ref named-templ-param "Named parameter" for setting
    /// \c LargeValue type.
    ///
    /// \ref named-templ-param "Named parameter" for setting \c LargeValue
    /// type. It is used for internal computations in the algorithm.
    template <typename T>
    struct SetLargeValue
      : public Karp<GR, LEN, SetLargeValueTraits<T> > {
      typedef Karp<GR, LEN, SetLargeValueTraits<T> > Create;
    };

    template <typename T>
    struct SetPathTraits : public Traits {
      typedef T Path;
    };

    /// \brief \ref named-templ-param "Named parameter" for setting
    /// \c %Path type.
    ///
    /// \ref named-templ-param "Named parameter" for setting the \c %Path
    /// type of the found cycles.
    /// It must conform to the \ref lemon::concepts::Path "Path" concept
    /// and it must have an \c addFront() function.
    template <typename T>
    struct SetPath
      : public Karp<GR, LEN, SetPathTraits<T> > {
      typedef Karp<GR, LEN, SetPathTraits<T> > Create;
    };

    /// @}

  public:

    /// \brief Constructor.
    ///
    /// The constructor of the class.
    ///
    /// \param digraph The digraph the algorithm runs on.
    /// \param length The lengths (costs) of the arcs.
    Karp( const Digraph &digraph,
          const LengthMap &length ) :
      _gr(digraph), _length(length), _comp(digraph), _out_arcs(digraph),
      _cycle_length(0), _cycle_size(1), _cycle_node(INVALID),
      _cycle_path(NULL), _local_path(false), _data(digraph),
      INF(std::numeric_limits<LargeValue>::has_infinity ?
          std::numeric_limits<LargeValue>::infinity() :
          std::numeric_limits<LargeValue>::max())
    {}

    /// Destructor.
    ~Karp() {
      if (_local_path) delete _cycle_path;
    }

    /// \brief Set the path structure for storing the found cycle.
    ///
    /// This function sets an external path structure for storing the
    /// found cycle.
    ///
    /// If you don't call this function before calling \ref run() or
    /// \ref findMinMean(), it will allocate a local \ref Path "path"
    /// structure. The destuctor deallocates this automatically
    /// allocated object, of course.
    ///
    /// \note The algorithm calls only the \ref lemon::Path::addFront()
    /// "addFront()" function of the given path structure.
    ///
    /// \return <tt>(*this)</tt>
    Karp& cycle(Path &path) {
      if (_local_path) {
        delete _cycle_path;
        _local_path = false;
      }
      _cycle_path = &path;
      return *this;
    }

    /// \name Execution control
    /// The simplest way to execute the algorithm is to call the \ref run()
    /// function.\n
    /// If you only need the minimum mean length, you may call
    /// \ref findMinMean().

    /// @{

    /// \brief Run the algorithm.
    ///
    /// This function runs the algorithm.
    /// It can be called more than once (e.g. if the underlying digraph
    /// and/or the arc lengths have been modified).
    ///
    /// \return \c true if a directed cycle exists in the digraph.
    ///
    /// \note <tt>mmc.run()</tt> is just a shortcut of the following code.
    /// \code
    ///   return mmc.findMinMean() && mmc.findCycle();
    /// \endcode
    bool run() {
      return findMinMean() && findCycle();
    }

    /// \brief Find the minimum cycle mean.
    ///
    /// This function finds the minimum mean length of the directed
    /// cycles in the digraph.
    ///
    /// \return \c true if a directed cycle exists in the digraph.
    bool findMinMean() {
      // Initialization and find strongly connected components
      init();
      findComponents();
      
      // Find the minimum cycle mean in the components
      for (int comp = 0; comp < _comp_num; ++comp) {
        if (!initComponent(comp)) continue;
        processRounds();
        updateMinMean();
      }
      return (_cycle_node != INVALID);
    }

    /// \brief Find a minimum mean directed cycle.
    ///
    /// This function finds a directed cycle of minimum mean length
    /// in the digraph using the data computed by findMinMean().
    ///
    /// \return \c true if a directed cycle exists in the digraph.
    ///
    /// \pre \ref findMinMean() must be called before using this function.
    bool findCycle() {
      if (_cycle_node == INVALID) return false;
      IntNodeMap reached(_gr, -1);
      int r = _data[_cycle_node].size();
      Node u = _cycle_node;
      while (reached[u] < 0) {
        reached[u] = --r;
        u = _gr.source(_data[u][r].pred);
      }
      r = reached[u];
      Arc e = _data[u][r].pred;
      _cycle_path->addFront(e);
      _cycle_length = _length[e];
      _cycle_size = 1;
      Node v;
      while ((v = _gr.source(e)) != u) {
        e = _data[v][--r].pred;
        _cycle_path->addFront(e);
        _cycle_length += _length[e];
        ++_cycle_size;
      }
      return true;
    }

    /// @}

    /// \name Query Functions
    /// The results of the algorithm can be obtained using these
    /// functions.\n
    /// The algorithm should be executed before using them.

    /// @{

    /// \brief Return the total length of the found cycle.
    ///
    /// This function returns the total length of the found cycle.
    ///
    /// \pre \ref run() or \ref findMinMean() must be called before
    /// using this function.
    LargeValue cycleLength() const {
      return _cycle_length;
    }

    /// \brief Return the number of arcs on the found cycle.
    ///
    /// This function returns the number of arcs on the found cycle.
    ///
    /// \pre \ref run() or \ref findMinMean() must be called before
    /// using this function.
    int cycleArcNum() const {
      return _cycle_size;
    }

    /// \brief Return the mean length of the found cycle.
    ///
    /// This function returns the mean length of the found cycle.
    ///
    /// \note <tt>alg.cycleMean()</tt> is just a shortcut of the
    /// following code.
    /// \code
    ///   return static_cast<double>(alg.cycleLength()) / alg.cycleArcNum();
    /// \endcode
    ///
    /// \pre \ref run() or \ref findMinMean() must be called before
    /// using this function.
    double cycleMean() const {
      return static_cast<double>(_cycle_length) / _cycle_size;
    }

    /// \brief Return the found cycle.
    ///
    /// This function returns a const reference to the path structure
    /// storing the found cycle.
    ///
    /// \pre \ref run() or \ref findCycle() must be called before using
    /// this function.
    const Path& cycle() const {
      return *_cycle_path;
    }

    ///@}

  private:

    // Initialization
    void init() {
      if (!_cycle_path) {
        _local_path = true;
        _cycle_path = new Path;
      }
      _cycle_path->clear();
      _cycle_length = 0;
      _cycle_size = 1;
      _cycle_node = INVALID;
      for (NodeIt u(_gr); u != INVALID; ++u)
        _data[u].clear();
    }

    // Find strongly connected components and initialize _comp_nodes
    // and _out_arcs
    void findComponents() {
      _comp_num = stronglyConnectedComponents(_gr, _comp);
      _comp_nodes.resize(_comp_num);
      if (_comp_num == 1) {
        _comp_nodes[0].clear();
        for (NodeIt n(_gr); n != INVALID; ++n) {
          _comp_nodes[0].push_back(n);
          _out_arcs[n].clear();
          for (OutArcIt a(_gr, n); a != INVALID; ++a) {
            _out_arcs[n].push_back(a);
          }
        }
      } else {
        for (int i = 0; i < _comp_num; ++i)
          _comp_nodes[i].clear();
        for (NodeIt n(_gr); n != INVALID; ++n) {
          int k = _comp[n];
          _comp_nodes[k].push_back(n);
          _out_arcs[n].clear();
          for (OutArcIt a(_gr, n); a != INVALID; ++a) {
            if (_comp[_gr.target(a)] == k) _out_arcs[n].push_back(a);
          }
        }
      }
    }

    // Initialize path data for the current component
    bool initComponent(int comp) {
      _nodes = &(_comp_nodes[comp]);
      int n = _nodes->size();
      if (n < 1 || (n == 1 && _out_arcs[(*_nodes)[0]].size() == 0)) {
        return false;
      }      
      for (int i = 0; i < n; ++i) {
        _data[(*_nodes)[i]].resize(n + 1, PathData(INF));
      }
      return true;
    }

    // Process all rounds of computing path data for the current component.
    // _data[v][k] is the length of a shortest directed walk from the root
    // node to node v containing exactly k arcs.
    void processRounds() {
      Node start = (*_nodes)[0];
      _data[start][0] = PathData(0);
      _process.clear();
      _process.push_back(start);

      int k, n = _nodes->size();
      for (k = 1; k <= n && int(_process.size()) < n; ++k) {
        processNextBuildRound(k);
      }
      for ( ; k <= n; ++k) {
        processNextFullRound(k);
      }
    }

    // Process one round and rebuild _process
    void processNextBuildRound(int k) {
      std::vector<Node> next;
      Node u, v;
      Arc e;
      LargeValue d;
      for (int i = 0; i < int(_process.size()); ++i) {
        u = _process[i];
        for (int j = 0; j < int(_out_arcs[u].size()); ++j) {
          e = _out_arcs[u][j];
          v = _gr.target(e);
          d = _data[u][k-1].dist + _length[e];
          if (_tolerance.less(d, _data[v][k].dist)) {
            if (_data[v][k].dist == INF) next.push_back(v);
            _data[v][k] = PathData(d, e);
          }
        }
      }
      _process.swap(next);
    }

    // Process one round using _nodes instead of _process
    void processNextFullRound(int k) {
      Node u, v;
      Arc e;
      LargeValue d;
      for (int i = 0; i < int(_nodes->size()); ++i) {
        u = (*_nodes)[i];
        for (int j = 0; j < int(_out_arcs[u].size()); ++j) {
          e = _out_arcs[u][j];
          v = _gr.target(e);
          d = _data[u][k-1].dist + _length[e];
          if (_tolerance.less(d, _data[v][k].dist)) {
            _data[v][k] = PathData(d, e);
          }
        }
      }
    }

    // Update the minimum cycle mean
    void updateMinMean() {
      int n = _nodes->size();
      for (int i = 0; i < n; ++i) {
        Node u = (*_nodes)[i];
        if (_data[u][n].dist == INF) continue;
        LargeValue length, max_length = 0;
        int size, max_size = 1;
        bool found_curr = false;
        for (int k = 0; k < n; ++k) {
          if (_data[u][k].dist == INF) continue;
          length = _data[u][n].dist - _data[u][k].dist;
          size = n - k;
          if (!found_curr || length * max_size > max_length * size) {
            found_curr = true;
            max_length = length;
            max_size = size;
          }
        }
        if ( found_curr && (_cycle_node == INVALID ||
             max_length * _cycle_size < _cycle_length * max_size) ) {
          _cycle_length = max_length;
          _cycle_size = max_size;
          _cycle_node = u;
        }
      }
    }

  }; //class Karp

  ///@}

} //namespace lemon

#endif //LEMON_KARP_H
