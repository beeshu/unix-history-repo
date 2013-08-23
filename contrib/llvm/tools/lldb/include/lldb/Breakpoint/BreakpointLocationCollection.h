//===-- BreakpointLocationCollection.h --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_BreakpointLocationCollection_h_
#define liblldb_BreakpointLocationCollection_h_

// C Includes
// C++ Includes
#include <vector>
// Other libraries and framework includes
// Project includes
#include "lldb/lldb-private.h"

namespace lldb_private {

class BreakpointLocationCollection
{
public:
    BreakpointLocationCollection();

    ~BreakpointLocationCollection();

    //------------------------------------------------------------------
    /// Add the breakpoint \a bp_loc_sp to the list.
    ///
    /// @param[in] bp_sp
    ///     Shared pointer to the breakpoint location that will get added
    ///     to the list.
    ///
    /// @result
    ///     Returns breakpoint location id.
    //------------------------------------------------------------------
    void
    Add (const lldb::BreakpointLocationSP& bp_loc_sp);

    //------------------------------------------------------------------
    /// Removes the breakpoint location given by \b breakID from this
    /// list.
    ///
    /// @param[in] break_id
    ///     The breakpoint index to remove.
    ///
    /// @param[in] break_loc_id
    ///     The breakpoint location index in break_id to remove.
    ///
    /// @result
    ///     \b true if the breakpoint was in the list.
    //------------------------------------------------------------------
    bool
    Remove (lldb::break_id_t break_id, lldb::break_id_t break_loc_id);

    //------------------------------------------------------------------
    /// Returns a shared pointer to the breakpoint location with id \a
    /// breakID.
    ///
    /// @param[in] break_id
    ///     The breakpoint  ID to seek for.
    ///
    /// @param[in] break_loc_id
    ///     The breakpoint location ID in \a break_id to seek for.
    ///
    /// @result
    ///     A shared pointer to the breakpoint.  May contain a NULL
    ///     pointer if the breakpoint doesn't exist.
    //------------------------------------------------------------------
    lldb::BreakpointLocationSP
    FindByIDPair (lldb::break_id_t break_id, lldb::break_id_t break_loc_id);

    //------------------------------------------------------------------
    /// Returns a shared pointer to the breakpoint location with id \a
    /// breakID, const version.
    ///
    /// @param[in] breakID
    ///     The breakpoint location ID to seek for.
    ///
    /// @param[in] break_loc_id
    ///     The breakpoint location ID in \a break_id to seek for.
    ///
    /// @result
    ///     A shared pointer to the breakpoint.  May contain a NULL
    ///     pointer if the breakpoint doesn't exist.
    //------------------------------------------------------------------
    const lldb::BreakpointLocationSP
    FindByIDPair (lldb::break_id_t break_id, lldb::break_id_t break_loc_id) const;

    //------------------------------------------------------------------
    /// Returns a shared pointer to the breakpoint location with index
    /// \a i.
    ///
    /// @param[in] i
    ///     The breakpoint location index to seek for.
    ///
    /// @result
    ///     A shared pointer to the breakpoint.  May contain a NULL
    ///     pointer if the breakpoint doesn't exist.
    //------------------------------------------------------------------
    lldb::BreakpointLocationSP
    GetByIndex (size_t i);

    //------------------------------------------------------------------
    /// Returns a shared pointer to the breakpoint location with index
    /// \a i, const version.
    ///
    /// @param[in] i
    ///     The breakpoint location index to seek for.
    ///
    /// @result
    ///     A shared pointer to the breakpoint.  May contain a NULL
    ///     pointer if the breakpoint doesn't exist.
    //------------------------------------------------------------------
    const lldb::BreakpointLocationSP
    GetByIndex (size_t i) const;

    //------------------------------------------------------------------
    /// Returns the number of elements in this breakpoint location list.
    ///
    /// @result
    ///     The number of elements.
    //------------------------------------------------------------------
    size_t
    GetSize() const { return m_break_loc_collection.size(); }

    //------------------------------------------------------------------
    /// Enquires of all the breakpoint locations in this list whether
    /// we should stop at a hit at \a breakID.
    ///
    /// @param[in] context
    ///    This contains the information about this stop.
    ///
    /// @param[in] breakID
    ///    This break ID that we hit.
    ///
    /// @return
    ///    \b true if we should stop, \b false otherwise.
    //------------------------------------------------------------------
    bool
    ShouldStop (StoppointCallbackContext *context);

    //------------------------------------------------------------------
    /// Print a description of the breakpoint locations in this list
    /// to the stream \a s.
    ///
    /// @param[in] s
    ///     The stream to which to print the description.
    ///
    /// @param[in] level
    ///     The description level that indicates the detail level to
    ///     provide.
    ///
    /// @see lldb::DescriptionLevel
    //------------------------------------------------------------------
    void GetDescription (Stream *s, lldb::DescriptionLevel level);
    
    //------------------------------------------------------------------
    /// Check whether this collection of breakpoint locations have any
    /// thread specifiers, and if yes, is \a thread_id contained in any
    /// of these specifiers.
    ///
    /// @param[in] thread
    ///     The thread against which to test.
    ///
    /// return
    ///     \b true if the collection contains at least one location that
    ///     would be valid for this thread, false otherwise.
    //------------------------------------------------------------------
    bool ValidForThisThread (Thread *thread);

    //------------------------------------------------------------------
    /// Tell whether ALL the breakpoints in the location collection are internal.
    ///
    /// @result
    ///     \b true if all breakpoint locations are owned by internal breakpoints,
    ///     \b false otherwise.
    //------------------------------------------------------------------
    bool IsInternal() const;


protected:
    //------------------------------------------------------------------
    // Classes that inherit from BreakpointLocationCollection can see
    // and modify these
    //------------------------------------------------------------------

private:
    //------------------------------------------------------------------
    // For BreakpointLocationCollection only
    //------------------------------------------------------------------

    typedef std::vector<lldb::BreakpointLocationSP> collection;

    collection::iterator
    GetIDPairIterator(lldb::break_id_t break_id, lldb::break_id_t break_loc_id);

    collection::const_iterator
    GetIDPairConstIterator(lldb::break_id_t break_id, lldb::break_id_t break_loc_id) const;

    collection m_break_loc_collection;

};

} // namespace lldb_private

#endif  // liblldb_BreakpointLocationCollection_h_
