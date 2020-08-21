/*
 *  COPYRIGHT (c) 2012 Chris Johns <chrisj@rtems.org>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */
/**
 * @file
 *
 * @ingroup rtems_rtld
 *
 * @brief RTEMS Run-Time Link Editor Chain Iterator
 *
 * A means of executing an iterator on a chain.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>

#include "rtl-chain-iterator.h"

bool
rtems_rtl_chain_iterate (List_t*              chain,
                         rtems_chain_iterator iterator,
                         void*                data)
{
  ListItem_t* node = listGET_HEAD_ENTRY (chain);
  while (listGET_END_MARKER(chain) != node)
  {
    ListItem_t* next_node = listGET_NEXT (node);
    if (!iterator (node, data))
      return false;
    node = next_node;
  }
  return true;
}

/**
 * Count iterator.
 */
static bool
rtems_rtl_count_iterator (ListItem_t* node, void* data)
{
  int* count = data;
  ++(*count);
  return true;
}

int
rtems_rtl_chain_count (List_t* chain)
{
  int count = 0;
  rtems_rtl_chain_iterate (chain, rtems_rtl_count_iterator, &count);
  return count;
}
