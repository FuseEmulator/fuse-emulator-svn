10 PRINT "123456"( 3 TO 5 )
20 PRINT "123456"( 2 TO )
30 PRINT "123456"( TO 4 )
40 PRINT "123456"( TO )
50 PRINT "123456"( 2 TO 1 )
60 PRINT "123456"( 2 TO 0 )

110 LET s$ = "123456"
120 LET s$( 2 TO 3 ) = "AB": PRINT "<";s$;">"
130 LET s$( 3 TO ) = "CD": PRINT "<";s$;">"
140 LET s$( 1 TO 3 ) = "EFGHIJ": PRINT "<";s$;">"
150 LET s$( TO ) = "K": PRINT "<";s$;">"
