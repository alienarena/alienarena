Object-oriented layer - Code generator
---------------------------------------

	I. Declaration syntax
	II. Known issues

The ool-gen.pl Perl script can be used to generate C headers and skeleton
code for AlienArena's object-oriented layer (source/qcommon/objects.{c,h})
from a bastardised form of C++ class declaration.


I. Declaration syntax
----------------------

Class declarations are read from files with the .cdf extension. Each file
is expected to contain one (and only one) class declaration.

C++-like comments are skipped, while C-like comments are kept and used in the
generated files.

It is possible to include other class declarations (e.g. for inheritance)
using #include "path/to/cdf". Of course, the class declaration files must be
kept around for this to work. The only exception is "qcommon/objects" which
is defined internally by the script (for the OOL_Object base class).

A class declaration follows the syntax below:

!classdecl!	::=	class <classname> !parentclass! { !classblock! };

!parentclass!	::=	: <classname>
		|

If there is no explicit parent class, OOL_Object will be used.

!classblock!	::=	!classitem! !classblock!
		|	!classitem!

!classitem!	::=	!scope! :
		|	!classmember! ;

!scope!		::=	public
		|	private

The "protected" scope is also supported, but using it and using "public" have
the same result. The default scope is "private".

While the OOL's code does not really support public vs. private members, it
still has effects:
 * private methods and static fields will only be defined in the C skeleton;
 * private fields will be declared in the class structure, but their name
will receive an underscore as a prefix.

!classmember!	::=	!fieldorprop!
		|	!staticfield!
		|	!method!

!fieldorprop!	::=	!field!
		|	!field! !proptype!

While properties may be defined in both "public" and "private" scopes, they
will always be considered as being public.

!staticfield!	::=	static !field! = <initial value>

!field!		::=	<type> <name>
		|	<type> <name> !arraydecl!

!arraydecl!	::=	!arraydim! !arraydecl!
		|	!arraydim!

!arraydim	::=	[ <anything> ]

!proptype!	::=	| type <proptype> !propopts! |
		::=	| custom !propopts! |

The type of a property must be one of the OOL_PTYPE_ constants without the
prefix (e.g. WORD)

!propopts!	::=	!propopt! !propopts!
		|	!propopt!

!propopt!	::=	validator
		|	change
		|	free
		|

Property options indicate which custom handlers should be generated.
The "free" option is only available for custom properties.

!method!	::=	static !inline! !methdecl!
		|	!inline! !methdecl!
		|	virtual !methdecl! !abstract!

!inline!	::=	inline
		|

!abstract!	::=	= 0
		|

!methdecl!	::=	<type> <name> ( !methargs! )

!methargs!	::=	!arglist!
		|	void
		|

!arglist!	::=	!arg! , !arglist!
		|	!arg!

!arg!		::=	<type> <name>

The name of an argument must never be "object", as this is reserved for the
object being manipulated in non-static methods.


Regarding inheritance...

 * It is possible (albeit not recommended) to create private fields or
methods with the same name in a class' hierarchy.

 * If at some point a field is declared as being public, then it is no longer
possible to redeclare it as private.

 * The initial declaration of a public method determines whether the method is
polymorphic or not (using "virtual").

 * If a public method is initially declared as static or non-polymorphic,
then it isn't possible to create a method with the same name in any of the
child classes.

 * It is only possible to override a method declared as virtual in its
initial declaration class. However, the "virtual" keyword becomes optional in
child classes.

 * An abstract method can only be declared in the method's initial
declaration class, not when overriding.


Regarding code generation...

 * The generated header includes both the class and instance structures, as
well as the class-defining function declaration. It will also contain
declarations for public non-polymorphic or static methods, as well as
inline wrappers for new polymorphic methods, and include skeletton blocks for
any public, inline method.

 * The generated C source will contain forward declarations and empty
definitions for all non-inline methods as well as property handlers, as well
as the full code for the class-defining function.


II. Known issues

 * Fields / static fields / properties containing function pointers are not
supported.

 * Arrays are not supported.

 * Any header inclusion that does not correspond to an OOL class needs to be
added manually.

 * Trying to initialise a static member with a string that contains the ';'
character will fail.

 * While there is a "-r" option to set the root directory in which files are
to be found, it does not work properly. The script should be executed from
the root directory instead.

 * The script's code is terribly ugly.
