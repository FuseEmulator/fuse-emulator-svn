#include "test.h"

static test_edge_sequence_t
complete_edges_list[] = 
{
  /* Standard speed data block */
  { 2168, 3223, 0 },	/* Pilot */
  {  667,    1, 0 },	/* Sync 1 */
  {  735,    1, 0 },	/* Sync 2 */

  { 1710,    2, 0 },	/* Bit 1 */
  {  855,    2, 0 },	/* Bit 2 */
  { 1710,    2, 0 },	/* Bit 3 */
  {  855,    2, 0 },	/* Bit 4 */
  { 1710,    2, 0 },	/* Bit 5 */
  {  855,    2, 0 },	/* Bit 6 */
  { 1710,    2, 0 },	/* Bit 7 */
  {  855,    2, 0 },	/* Bit 8 */

  { 8194368, 1, 1 },	/* Pause */

  /* Turbo speed data block */
  { 1000,    5, 0 },	/* Pilot */
  {  123,    1, 0 },	/* Sync 1 */
  {  456,    1, 0 },	/* Sync 2 */

  {  789,   16, 0 },	/* Byte 1, bits 1-8 */
  {  400,   16, 0 },	/* Byte 2, bits 1-8 */

  {  789,    2, 0 },	/* Byte 3, bit 1 */
  {  400,    2, 0 },	/* Byte 3, bit 2 */
  {  789,    2, 0 },	/* Byte 3, bit 3 */
  {  400,    2, 0 },	/* Byte 3, bit 4 */
  {  789,    2, 0 },	/* Byte 3, bit 5 */
  {  400,    2, 0 },	/* Byte 3, bit 6 */
  {  789,    2, 0 },	/* Byte 3, bit 7 */
  {  400,    2, 0 },	/* Byte 3, bit 8 */

  {  400,    2, 0 },	/* Byte 4, bit 1 */
  {  789,    2, 0 },	/* Byte 4, bit 2 */
  {  400,    2, 0 },	/* Byte 4, bit 3 */
  {  789,    2, 0 },	/* Byte 4, bit 4 */

  { 3448972, 1, 1 },	/* Pause */

  /* Pure tone block */
  {  535,  665, 0 },
  {  535,    1, 1 },

  /* List of pulses */
  {  772,    1, 0 },
  {  297,    1, 0 },
  {  692,    1, 1 },

  /* Pure data block */
  { 1639,   16, 0 },	/* Byte 1, bits 1-8 */
  {  552,   16, 0 },	/* Byte 2, bits 1-8 */
  { 1639,   12, 0 },	/* Byte 3, bits 1-6 */
  { 1935897, 1, 1 },	/* Pause */

  /* Pause block */
  { 2159539, 1, 3 },

  { 0, 0 }		/* End marker */

};

test_return_t
test_15( void )
{
  return check_edges( "complete-tzx.tzx", complete_edges_list );
}

