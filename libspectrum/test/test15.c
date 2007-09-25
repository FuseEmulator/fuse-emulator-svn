#include "test.h"

static test_edge_sequence_t
complete_edges_list[] = 
{
  /* Standard speed data block */
  { 2168, 3223 },	/* Pilot */
  {  667,    1 },	/* Sync 1 */
  {  735,    1 },	/* Sync 2 */

  { 1710,    2 },	/* Bit 1 */
  {  855,    2 },	/* Bit 2 */
  { 1710,    2 },	/* Bit 3 */
  {  855,    2 },	/* Bit 4 */
  { 1710,    2 },	/* Bit 5 */
  {  855,    2 },	/* Bit 6 */
  { 1710,    2 },	/* Bit 7 */
  {  855,    2 },	/* Bit 8 */

  { 8194368, 1 },	/* Pause */

  /* Turbo speed data block */
  { 1000,    5 },	/* Pilot */
  {  123,    1 },	/* Sync 1 */
  {  456,    1 },	/* Sync 2 */

  {  789,   16 },	/* Byte 1, bits 1-8 */
  {  400,   16 },	/* Byte 2, bits 1-8 */

  {  789,    2 },	/* Byte 3, bit 1 */
  {  400,    2 },	/* Byte 3, bit 2 */
  {  789,    2 },	/* Byte 3, bit 3 */
  {  400,    2 },	/* Byte 3, bit 4 */
  {  789,    2 },	/* Byte 3, bit 5 */
  {  400,    2 },	/* Byte 3, bit 6 */
  {  789,    2 },	/* Byte 3, bit 7 */
  {  400,    2 },	/* Byte 3, bit 8 */

  {  400,    2 },	/* Byte 4, bit 1 */
  {  789,    2 },	/* Byte 4, bit 2 */
  {  400,    2 },	/* Byte 4, bit 3 */
  {  789,    2 },	/* Byte 4, bit 4 */

  { 0, 0 }		/* End marker */

};

test_return_t
test_15( void )
{
  return check_edges( "complete-tzx.tzx", complete_edges_list );
}

