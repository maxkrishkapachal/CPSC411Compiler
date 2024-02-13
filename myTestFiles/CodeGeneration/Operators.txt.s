Loaded: /home/profs/aycock/411/sw/share/spim/exceptions.s
And &&
-------
bool red: true
bool blue: true
red && blue: true
bool blue: false
red && blue: false
-------
Or ||
-------
bool red: true
bool blue: false
red || blue: true
bool blue: true
red || blue: true
bool red: false
bool blue: false
red || blue: false
-------
Not !
-------
bool blue: true
!blue: false
-------
Mult *
-------
4 * 2 = 8
-------
Div /
-------
4 / 2 = 2
-------
Rem %
-------
4 % 2 = 0
-------
Plus +
-------
4 + 2 = 6
-------
Minus -
-------
4 - 2 = 2
-------
Negation -
-------
green: -3
-green: 3
-------
Equal ==
-------
int green: 3
int purple: 3
green == purple: true
purple: 4
purple == green: false
-------
Not Equal !=
-------
int green: 4
int purple: 3
green != purple: true
int purple: 4
purple != green: false
-------
Greater Than >
-------
int green: 2
int purple: 3
purple > green: true
-------
Greater Than or Equal >=
-------
int green: 2
int purple: 3
purple >= green: true
int green: 3
purple >= green: true
-------
Less Than <
-------
int green: 3
int purple: 2
purple < green: true
-------
Less Than or Equal <=
-------
int green: 3
int purple: 2
purple <= green: true
int green: 2
purple <= green: true