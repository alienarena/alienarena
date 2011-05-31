#!/usr/bin/env perl
#
# Copyright (C) 2010 COR Entertainment
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# 
# See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
##
#
# This script is used as a code generator for the object-oriented
# layer (source/qcommon/objects.{c,h}).
#
# See README.txt for more details

use strict;
use File::Basename qw( basename );
use File::Copy;


###
# printUsage $retCode
#
# Print a short text about how the script works, and exit with the
# specified return code.
###
sub printUsage($)
{
	print STDERR <<'EOL';
AlienArena OO layer code generator

Usage:
	ool-gen.pl -h
	ool-gen.pl -g [-r <root directory>] [-f] <definition file>

Commands:
	-h	Print this message
	-g	Generate C source code from a definition file

Options:
	-r	Specify the source code's root directory
	-f	Force generation, overwriting any existing files

EOL
	exit(shift);
}


###
# getCommandLine @arguments
#
# Read the command line arguments and try to understand what the script
# is supposed to do and how. Return a hash reference to the current
# configuration if the arguments are correct, do not return otherwise.
###
sub getCommandLine(@)
{
	my $config = { };
	printUsage(1) unless @_;

	# Get command
	my $command = shift;
	my $nArgs;
	printUsage(1) unless $command =~ /^-[a-z]$/;
	printUsage(0) if $command eq '-h';

	if ( $command eq '-g' ) {
		$config->{action} = \&generate;
		$nArgs = 1;
	} else {
		printUsage(1);
	}

	# Read options
	my $optDefs = {
		'-r'	=> [ 'root' , 1 , '.' , undef ] ,
		'-f'	=> [ 'force' , 0 , 0 , 1 ] ,
	};
	my $waitOpt = 0;
	my $curOpt;
	$config->{target} = [];
	while ( @_ ) {
		my $arg = shift;

		if ( $waitOpt == 0 ) {
			if ( defined $optDefs->{ $arg } ) {
				printUsage(1) if defined $config->{ $optDefs->{ $arg }[ 0 ] };
				$waitOpt = $optDefs->{ $arg }[ 1 ];
				$config->{ $optDefs->{ $arg }[ 0 ] } = $optDefs->{ $arg }[ 3 ];
				$curOpt = $arg;
			} else {
				printUsage(1) unless $nArgs;
				push @{$config->{target}} , $arg;
				$nArgs --;
			}
		} else {
			$config->{ $optDefs->{ $curOpt }[ 0 ] } = $arg;
			$waitOpt = 0;
		}
	}
	printUsage(1) if ( $nArgs || $waitOpt );

	# Set default values for options that weren't on the command line
	foreach my $def ( values %{ $optDefs } ) {
		next if defined $config->{ $def->[0] };
		$config->{ $def->[0] } = $def->[ 2 ];
	}

	return $config;
}


###
# syntaxError( $file , $message )
#
# Die with a syntax error message.
###
sub syntaxError
{
	my ( $file, $message) = @_;
	die "Syntax error: $message (in $file)\n";
}


###
# findClass( $data , $cName )
#
# Find the file that contains the specified class.
# Return '' when not found
###
sub findClass
{
	my ( $data , $cName ) = @_;
	foreach my $fName ( keys %$data ) {
		return $fName if ( defined( $data->{$fName}{class} ) && $data->{$fName}{class} eq $cName );
	}
	return '';
}


###
# parseClassMember( $data , $bName , $scope , $contents )
#
# Parse a class member declaration. Return the contents with the class
# member removed.
#
# A class member is one of
#	static typespec name = ...;
#	typespec name;
#	typespec name |specialstuff|;
#	virtual typespec name( parameters );
#	virtual typespec name( parameters ) = 0;
#	static typespec name( parameters );
#	typespec name( parameters );
# where
#	typespec is "word ( \* | word )*"
#	parameters is "(nothing) | void | parameter | parameter, parameters"
#	parameter is "typespec name"
#	specialstuff is "type TYPENAME prophandlers | custom prophandlers [free]"
#	prophandlers one or two of "validator" and "change"
#
# FIXME: add support for function pointers
###
sub parseClassMember
{
	my ( $data , $bName , $scope , $contents ) = @_;
	my $fData = $data->{$bName};

	# Handle static & virtual qualifiers
	my $virtual = 0;
	my $static = 0;
	my $inline = 0;
	while ( $contents =~ s/^(virtual|static|inline)\s+//s ) {
		syntaxError( "$bName.cdf" , "member can't be static AND virtual" )
			if ( $1 ne 'inline' && ( $static || $virtual ) );
		eval "\$$1 = 1;";
	}

	# Get type spec and name
	syntaxError( "$bName.cdf" , "type and name of member not found" )
		unless $contents =~ s/^(\w+(\s*\*|\s*\w+)*\w+)//s;
	my $typeSpec = $1;
	$typeSpec =~ s/\s*(\w+)$//;
	my $mName = $1;

	# Make sure the name is unique in the current class
	syntaxError( "$bName.cdf" , "member '$mName' declared more than once" )
		if defined $fData->{members}{$mName};

	# Expect end of member, assignment, property stuff, or parameters
	my ( $mType , $mData );
	syntaxError( "$bName.cdf" , "expecting ';', '|' , '=' , '[' or '(' after member name $mName" )
		unless $contents =~ s/\s*([;\(\|=\[])//s;
	my $found = $1;

	my $arrayDims = [];
	while ( $found eq '[' ) {
		syntaxError( "$bName.cdf" , "expected array dimension for $mName" )
			unless $contents =~ s/([^\]]+)\]//;
		push @$arrayDims , $1;
		syntaxError( "$bName.cdf" , "expecting ';', '|' , '=' or '[' after array dimension of $mName" )
			unless $contents =~ s/\s*([;\|=\[])//s;
		$found = $1;
	}

	syntaxError( "$bName.cfg" , "only methods can be declared 'inline'" )
		if ( $inline && $found ne '(' );

	if ( $found eq ';' ) {
		# End of line - valid if neither static nor virtual
		syntaxError( "$bName.cdf" , 'uninitialised field ' . $mName . ' cannot be static or virtual' )
			if ( $static || $virtual );

		$mType = 'fields';
		$mData = { type => $typeSpec };

	} elsif ( $found eq '=' ) {
		# Assignment - must be static
		# XXX: will not work for strings containing ';'
		syntaxError( "$bName.cdf" , 'assignment following non-static member ' . $mName )
			unless $static;
		syntaxError( "$bName.cdf" , 'unable to parse initial value of ' . $mName )
			unless $contents =~ s/^\s*([^;]+);//s;

		$mType = 'cfields';
		$mData = { type => $typeSpec , init => $1 };

	} elsif ( $found eq '|' ) {
		# Property details
		syntaxError( "$bName.cdf" , 'property ' . $mName . ' cannot be static or virtual' )
			if ( $static || $virtual );
		$mType = 'props';
		$mData = { type => $typeSpec , validator => 0 , change => 0 , free => 0 };

		syntaxError( "$bName.cdf" , 'invalid property type for ' . $mName )
			unless $contents =~ s/^\s*(type\s+[A-Z_]+|custom)(\s|\|)/$2/s;
		my $pSpec = $1;
		$mData->{custom} = $pSpec eq 'custom';
		if ( $mData->{custom} ) {
			$mData->{pType} = "OOL_PTYPE_CUSTOM";
		} else {
			$pSpec =~ s/^type\s+//s;
			$mData->{pType} = "OOL_PTYPE_" . $pSpec;
		}

		# Parse property qualifiers
		while ( $contents !~ /^\s*\|/s ) {
			syntaxError( "$bName.cdf" , 'invalid property qualifiers for ' . $mName )
				unless $contents =~ s/^\s+(validator|change|free)(\s|\|)/$2/s;
			syntaxError( "$bName.cdf" , "duplicate '$1' qualifier for property '$mName'" )
				if $mData->{$1};
			syntaxError( "$bName.cdf" , "property '$mName' is not custom but 'free' qualifier found" )
				if ( $1 eq 'free' && ! $mData->{custom} );
			$mData->{$1} = 1;
		}
		syntaxError( "$bName.cdf" , 'junk found at end of property ' . $mName )
			unless $contents =~ s/^\s*\|\s*;//s;

	} else {
		# Method
		$mType = 'methods';
		$mData = {
			retType	=> $typeSpec ,
			static	=> $static ,
			virtual	=> $virtual ,
			inline	=> $inline ,
		};

		my $params = [];
		if ( !( $contents =~ s/^(\s*void)?\s*\)//s ) ) {
			$contents = ','.$contents;
			while ( $contents =~ s/^\s*,\s*(\w+(\s*\*|\s*\w+)*\s*\w+)//s ) {
				my $pType = $1;
				$pType =~ s/\s*(\w+)$//s;
				push @{ $params } , { $1 => $pType };
			}
			syntaxError( "$bName.cdf" , 'found junk after parameters of method ' . $mName )
				unless $contents =~ s/^\s*\)//s;
		}
		$mData->{params} = $params;

		if ( $contents =~ s/^\s*=\s*0\s*;//s ) {
			# Abstract method
			syntaxError( "$bName.cdf" , 'abstract method ' . $mName . ' must be virtual' )
				unless $virtual;
			syntaxError( "$bName.cdf" , 'abstract method ' . $mName . ' must not be private' )
				if $scope eq 'private';
			$mData->{abstract} = 1;
		} elsif ( !( $contents =~ s/^\s*;//s ) ) {
			syntaxError( "$bName.cdf" , 'found junk after declaration of method ' . $mName );
		}
	}

	# Set scope according to location in class block; also, properties
	# cannot be private
	$mData->{public} = ( $scope ne 'private' && $mType ne 'props' );

	# Set array dimensions
	$mData->{arrayDims} = $arrayDims
		unless $mType eq 'methods';

	# Now we need to take a look at parent classes...
	# * Private methods and fields may share the name of a parent class'
	# private field or method.
	# * Public/protected fields and properties may only share the name of
	# a parent class' private field or method
	# * Public methods require checks about static/virtual/inline status
	# and their own "virtualness" may be updated in the process.
	if ( $mType ne 'methods' || !$mData->{public} ) {
		my $parent = findClass( $data , $fData->{parent} );
		while ( 1 ) {
			if ( defined $data->{$parent}{members}{$mName} ) {
				my $prType = $data->{$parent}{members}{$mName};
				my $prRec = $data->{$parent}{$prType}{$mName};
				syntaxError( "$bName.cdf" , "'$mName' overrides inherited public member from $data->{$parent}{class}" )
					if $prRec->{public};
				last;
			}
			last if not defined $data->{$parent}{parent};
			$parent = findClass( $data , $data->{$parent}{parent} );
		}
	} else {
		my ( $prType , $prRec , $prFound );
		my $parent = findClass( $data , $fData->{parent} );
		$prFound = 0;
		while ( 1 ) {
			if ( defined $data->{$parent}{members}{$mName} ) {
				$prType = $data->{$parent}{members}{$mName};
				$prRec = $data->{$parent}{$prType}{$mName};
				$prFound = 1;
				last;
			}
			last if not defined $data->{$parent}{parent};
			$parent = findClass( $data , $data->{$parent}{parent} );
		}

		if ( $prFound && $prRec->{public} ) {
			syntaxError( "$bName.cdf" , "'$mName' is redeclared as a method but wasn't in $data->{$parent}{class}" )
				unless $prType eq 'methods';

			if ( $mData->{static} ) {
				syntaxError( "$bName.cdf" , "'$mName' is redeclared as static, but wasn't in $data->{$parent}{class}" )
					unless $prRec->{static};
			} else {
				syntaxError( "$bName.cdf" , "'$mName' is redeclared as abstract" )
					if $mData->{abstract};
				syntaxError( "$bName.cdf" , "'$mName' is redeclared but wasn't virtual in $data->{$parent}{class}" )
					unless $prRec->{virtual};
				$mData->{virtual} = 1;
				$mData->{super} = $prRec->{super} if defined $prRec->{super};
				$mData->{onClass} = $data->{$parent}{class};
			}
		} else {
			$mData->{onClass} = $fData->{class};
		}
	}

	# Add comments, if there are any
	if ( @{ $fData->{__WORK__}{curcomments} } ) {
		$mData->{comment} = join( "" , @{ $fData->{__WORK__}{curcomments} } );
		$fData->{__WORK__}{curcomments} = [ ];
	}

	# Add member to class
	$fData->{members}{$mName} = $mType;
	$fData->{$mType}{$mName} = $mData;
	push @{$fData->{order}{$mType}} , $mName;

	return $contents;
}



###
# parseClassBlock( $data , $baseName , $fileContents )
#
# Parse the class block from the specified contents. Return the contents
# with the class block removed.
###
sub parseClassBlock
{
	my ( $data , $bName , $contents ) = @_;
	my $fData = $data->{$bName};
	my $wData = $fData->{__WORK__};

	# If there are pending comments, the last one is the class
	# comment, others are for the file itself.
	if ( @{ $wData->{curcomments} } ) {
		$fData->{classComment} = pop @{ $wData->{curcomments} };
		$fData->{fileComment} = join( "\n" , @{ $wData->{curcomments} } )
			if @{ $wData->{curcomments} };
		$wData->{curcomments} = [];
	}

	# We may have a ": parentclass" or a "{"
	if ( $contents =~ /^\s*:/s ) {
		$contents =~ s/^\s*:\s*//s;
		syntaxError( "$bName.cdf" , "expected class name after scope" )
			unless $contents =~ /^([A-Za-z][A-Za-z0-9_]*)/;
		my $pClass = $1;
		$contents =~ s/[A-Za-z][A-Za-z0-9_]*\s*//s;
		syntaxError( "$bName.cdf" , "parent class '$pClass' not found" )
			if findClass( $data , $pClass ) eq '';
		$fData->{parent} = $pClass;
	} else {
		$fData->{parent} = 'OOL_Object';
	}
	syntaxError( "$bName.cdf" , "expected { after class header" )
		unless $contents =~ /^\s*{/s;
	$contents =~ s/^\s*{//s;

	my $scope = 'private';
	while ( $contents ) {
		$contents =~ s/^\s+//s;

		# '};' -> end of class block
		last if ( $contents =~ s/^}\s*;//s );

		# Ignore // comments
		next if ( $contents =~ s/^\/\/.*?\n// );

		# Store /* */ comments in work area
		if ( $contents =~ /^\/\*/ ) {
			syntaxError( "$bName.cdf" , "expected '*/' before end of file" )
				unless $contents =~ s/^(\/\*.*?\*\/)//s;
			push @{ $data->{$bName}{__WORK__}{curcomments} } , $1;
			next;
		}

		# Handle scope
		if ( $contents =~ s/^(public|private|protected)\s*://s ) {
			$scope = $1;
			next;
		}

		$contents = parseClassMember( $data , $bName , $scope , $contents );
	}

	return $contents;
}


###
# parseConstantsBlock( $data , $baseName , $contents , $prefix )
#
# Parses a set of constant definitions
###
sub parseConstantsBlock
{
	my ( $data , $bName , $contents , $prefix ) = @_;

	while ( $contents ) {
		$contents =~ s/^\s+//s;

		# '};' -> end of class block
		last if ( $contents =~ s/^}\s*;//s );

		# Ignore // comments
		next if ( $contents =~ s/^\/\/.*?\n// );

		# Store /* */ comments in work area
		if ( $contents =~ /^\/\*/ ) {
			syntaxError( "$bName.cdf" , "expected '*/' before end of file" )
				unless $contents =~ s/^(\/\*.*?\*\/)//s;
			push @{ $data->{$bName}{__WORK__}{curcomments} } , $1;
			next;
		}

		# Get next constant definition
		syntaxError( "$bName.cdf" , "invalid constant definition" )
			unless $contents =~ s/(\w+)\s*=\s*([^;]+);//s;
		my ( $cName , $cValue ) = ( $1 , $2 );
		$cName = $prefix.$cName;
		syntaxError( "$bName.cdf" , "duplicate constant definition '$cName'" )
			if defined $data->{$bName}{consts}{$cName};

		my $cData = { value => $cValue };
		if ( @{ $data->{$bName}{__WORK__}{curcomments} } ) {
			$cData->{comments} = join( "\n" , @{ $data->{$bName}{__WORK__}{curcomments} } );
			$data->{$bName}{__WORK__}{curcomments} = [ ];
		}

		$data->{$bName}{consts}{$cName} = $cData;
		push @{ $data->{$bName}{order}{consts} } , $cName;
	}

	return $contents;
}


###
# parseDefinitionFile( $data , $file , $root )
#
# Parse a definition file, using the specified root directory when handling
# inclusions.
###
sub parseDefinitionFile
{
	my ( $data , $file , $root ) = @_;
	my $bName = $file;
	$bName =~ s/\.cdf$//;
	$bName =~ s/^$root\///;
	return if defined $data->{ $bName };

	# Read file contents
	open( my $INPUT , "<" , $file )
		or die "Could not open '$file'\n";
	my $contents = join("" , <$INPUT> );
	close( $INPUT );
	print STDERR "Parsing $file ...\n";

	# Initialise file data
	$data->{$bName} = {
		__WORK__ => {
			curcomments	=> []
		},
		include	=> [ ] ,
		headers	=> [ ] ,
		consts	=> { } ,
		members	=> { } ,
		methods	=> { } ,
		props	=> { } ,
		fields	=> { } ,
		cfields	=> { } ,
		order	=> {
			consts	=> [] ,
			methods	=> [] ,
			props	=> [] ,
			fields	=> [] ,
			cfields	=> [] ,
		} ,
	};

	while ( $contents ) {
		$contents =~ s/^\s+//;
		last unless $contents;

		# Ignore // comments
		next if ( $contents =~ s/^\/\/.*?\n// );

		# Store /* */ comments in work area
		if ( $contents =~ /^\/\*/ ) {
			syntaxError( $file , "expected '*/' before end of file" )
				unless $contents =~ s/^(\/\*.*?\*\/)//s;
			push @{ $data->{$bName}{__WORK__}{curcomments} } , $1;
			next;
		}

		# Handle includes
		if ( $contents =~ s/^#include\s+//s ) {
			syntaxError( $file , "expected \" or \< after #include" )
				unless $contents =~ s/^(["<])//;
			my $sep = ( $1 eq '"' ) ? '"' : '>';

			syntaxError( $file , "expected ${sep}filename${sep} after #include" )
				unless $contents =~ s/([\w\/\.]+)$sep//;
			my $iFile = $1;
			$iFile =~ s/\/\/+/\//g;

			if ( $iFile =~ /\.h$/ ) {
				push @{ $data->{$bName}{headers} } , $iFile;
			} else {
				syntaxError( $file , "trying to self-include" )
					if $iFile eq $bName;

				parseDefinitionFile( $data , "$root/$iFile.cdf" , $root );
				push @{ $data->{$bName}{include} } , $iFile;
			}
			next;
		}

		# Class definition block
		if ( $contents =~ s/^class\s+([A-Za-z]\w*)//s ) {
			my $cName = $1;
			syntaxError( $file , "trying to redefine class $cName" )
				unless findClass( $data , $cName ) eq '';
			$data->{$bName}{class} = $cName;

			$contents = parseClassBlock( $data , $bName , $contents );
			next;
		}

		# Constants definition block
		if ( $contents =~ s/^constants(\s+(\w+))?\s*{//s ) {
			$contents = parseConstantsBlock( $data , $bName , $contents , $2 );
			next;
		}

		$contents =~ s/^([^\n]+)(\n.*)?/$1/s;
		syntaxError( $file , "junk found: '$contents'" );
	}
}


###
# generateCHeader( $parsedData , $sourceFile , $root , $force )
#
# Generate the C header file for the class in the parsed file. Will fail
# if the file exists and $force is 0.
###
sub generateCHeader
{
	my ( $data , $sFile , $root , $force ) = @_;
	my $bName = $sFile;
	$bName =~ s/\.cdf$//;
	$bName =~ s/^$root\///;
	
	my $fData = $data->{$bName};
	my $headerFile = "$root/$bName.h";
	if ( -f $headerFile ) {
		if ( $force ) {
			move( $headerFile , "$headerFile.bak" );
		} else {
			print STDERR "$headerFile exists and will not be overwritten. Use -f to force.\n";
			return;
		}
	}

	open( my $fh , ">" , "$headerFile" )
		or die "Unable to create '$headerFile'\n";
	print "Creating $headerFile ...\n";

	# Get name for double-inclusion check
	my $dicName = uc( $bName );
	$dicName =~ s/\//_/g;
	$dicName = "__H_$dicName";

	# Generate header
	print $fh $fData->{fileComment} if defined $fData->{fileComment};
	print $fh <<"EOL";

/** \@file
 * \@brief $fData->{class} class declarations
 * \@note Initially generated from $sFile
 */
#ifndef $dicName
#define $dicName

EOL

	# Include other headers
	foreach my $included ( @{ $fData->{include} } ) {
		print $fh "#include \"$included.h\"\n";
	}
	foreach my $included ( @{ $fData->{header} } ) {
		print $fh "#include \"$included\"\n";
	}

	# Add "class comment" (should be a \defgroup)
	print $fh ( "\n\n" . $fData->{classComment} . "\n/*@\{*/\n" )
		if defined $fData->{classComment};

	# Constants
	foreach my $constName ( @{ $fData->{order}{consts} } ) {
		my $const = $fData->{consts}{$constName};
		print $fh "\n";
		print $fh ( $const->{comments} . "\n" )
			if defined $const->{comments};
		print $fh "#define $constName $const->{value}\n";
	}

	# Forward declaration & beginning of class structure
	print $fh <<"EOL";


/* Forward declaration of instance types */
struct $fData->{class}_s;
typedef struct $fData->{class}_s * $fData->{class};

/** \@brief Class structure for the $fData->{class} class
 */
struct $fData->{class}_cs
{
\t/** \@brief Parent class record */
\tstruct $fData->{parent}_cs parent;
EOL

	# Static fields
	foreach my $cFieldName ( @{ $fData->{order}{cfields} } ) {
		my $cField = $fData->{cfields}{$cFieldName};
		next unless $cField->{public};
		if ( defined $cField->{comment} ) {
			my $comment = $cField->{comment};
			$comment =~ s/\n\t+/\n\t/gs;
			print $fh "\n\t$comment";
		}
		print $fh "\n\t" . $cField->{type} . " " . $cFieldName;
		if ( @{$cField->{arrayDims}} ) {
			print $fh '[' . join('][' , @{$cField->{arrayDims}} ) . ']';
		}
		print $fh ";\n";
	}

	# Methods
	foreach my $methName ( @{ $fData->{order}{methods} } ) {
		my $method = $fData->{methods}{$methName};
		next unless $method->{virtual};
		next unless $method->{onClass} eq $fData->{class};

		if ( defined $method->{comment} ) {
			my $comment = $method->{comment};
			$comment =~ s/\n\t+/\n\t/gs;
			print $fh "\n\t$comment";
		}
		print $fh "\n\t" . $method->{retType} . " (* " . $methName . ")( ";

		my @params = ( { 'object' => $fData->{class} } , @{ $method->{params} } );
		my @pStr = ( );
		foreach my $mParam ( @params ) {
			my $pName = ( keys %$mParam )[ 0 ];
			my $pType = ( values %$mParam )[ 0 ];
			push @pStr , "$pType $pName";
		}
		print $fh join( " , " , @pStr ) . " );\n";
	}

	# End of class structure, beginning of instance structure
	print $fh <<"EOL";
};

/** \@brief Instance structure for the $fData->{class} class
 */
struct $fData->{class}_s
{
\t/** \@brief Parent instance */
\tstruct $fData->{parent}_s parent;
EOL

	# Properties
	foreach my $propName ( @{ $fData->{order}{props} } ) {
		my $prop = $fData->{props}{$propName};

		if ( defined $prop->{comment} ) {
			my $comment = $prop->{comment};
			$comment =~ s/\n\t+/\n\t/gs;
			print $fh "\n\t$comment";
		}
		print $fh "\n\t" . $prop->{type} . " " . $propName;
		if ( @{$prop->{arrayDims}} ) {
			print $fh '[' . join('][' , @{$prop->{arrayDims}} ) . ']';
		}
		print $fh ";\n";
	}

	# Fields
	foreach my $fieldName ( @{ $fData->{order}{fields} } ) {
		my $field = $fData->{fields}{$fieldName};

		if ( defined $field->{comment} ) {
			my $comment = $field->{comment};
			$comment =~ s/\n\t+/\n\t/gs;
			print $fh "\n\t$comment";
		}
		$fieldName = "_$fieldName" unless $field->{public};
		print $fh "\n\t" . $field->{type} . " " . $fieldName;
		if ( @{$field->{arrayDims}} ) {
			print $fh '[' . join('][' , @{$field->{arrayDims}} ) . ']';
		}
		print $fh ";\n";
	}

	# End of instance structure & class-defining function
	print $fh <<"EOL";
};


/** \@brief Defining function for the $fData->{class} class
 *
 * Initialise the class' definition if needed.
 *
 * \@return the class' definition
 */
OOL_Class $fData->{class}__Class( );

EOL

	# Public static method declarations
	foreach my $methName ( @{ $fData->{order}{methods} } ) {
		my $method = $fData->{methods}{$methName};
		next unless $method->{static} && $method->{public};

		if ( defined $method->{comment} ) {
			my $comment = $method->{comment};
			$comment =~ s/\n\t+/\n/gs;
			print $fh "\n$comment";
		}
		print $fh "\n";
		print $fh "static inline " if $method->{inline};
		print $fh $method->{retType} . " $fData->{class}_$methName(";

		my @params = @{ $method->{params} };
		my @pStr = ( );
		foreach my $mParam ( @params ) {
			my $pName = ( keys %$mParam )[ 0 ];
			my $pType = ( values %$mParam )[ 0 ];
			push @pStr , " $pType $pName";
		}
		print $fh join( " ," , @pStr ) . " )";

		if ( $method->{inline} ) {
			print $fh "\n{\n\t// XXX: write method code\n}\n\n";
		} else {
			print $fh ";\n\n";
		}
	}

	# Public method wrappers
	foreach my $methName ( @{ $fData->{order}{methods} } ) {
		my $method = $fData->{methods}{$methName};
		next if $method->{static};
		next unless $method->{onClass} eq $fData->{class};

		if ( defined $method->{comment} && !$method->{virtual} ) {
			my $comment = $method->{comment};
			$comment =~ s/\n\t+/\n/gs;
			print $fh "\n$comment";
		} elsif ( $method->{virtual} ) {
			print $fh "\n/** \@brief Wrapper for the \@link $fData->{class}_cs::$methName $methName \@endlink method */";
		}
		print $fh "\n" . ( ( $method->{inline} || $method->{virtual} ) ? "static inline " : "" )
			.  $method->{retType} . " $fData->{class}_$methName(";

		my @params = ( {
				'object' => ( ( $method->{inline} || $method->{virtual} ) ? 'OOL_Object' : $fData->{class} )
			} , @{ $method->{params} } );
		my @pStr = ( );
		foreach my $mParam ( @params ) {
			my $pName = ( keys %$mParam )[ 0 ];
			my $pType = ( values %$mParam )[ 0 ];
			push @pStr , " $pType $pName";
		}
		print $fh join( " ," , @pStr ) . " )";

		if ( $method->{inline} || $method->{virtual} ) {
			print $fh "\n{\n\t";
			if ( $method->{inline} ) {
				print $fh "// XXX: write method code\n";
			} else {
				print $fh "assert( OOL_GetClassAs( object , $fData->{class} )->$methName != NULL );\n\t"
					if ( $method->{abstract} );
				print $fh "return " unless $method->{retType} eq 'void';
				print $fh "OOL_GetClassAs( object , $fData->{class} )->$methName(";
				@pStr = ( " OOL_Cast( object , $fData->{class} )" );
				shift @params;
				foreach my $mParam ( @params ) {
					my $pName = ( keys %$mParam )[ 0 ];
					push @pStr , " $pName";
				}
				print $fh join( " ," , @pStr ) . " );\n";
			}
			print $fh "}\n\n";
		} else {
			print $fh ";\n\n";
		}
	}

	# End of header file
	print $fh ( "\n/*@\}*/\n" )
		if defined $fData->{classComment};
	print $fh "\n\n#endif /*$dicName*/\n";
	close $fh;
}



###
# generateCSource( $parsedData , $sourceFile , $root , $force )
#
# Generate the skeletton of the C source file for the class in the input file.
# Will fail if the file exists and $force is 0.
###
sub generateCSource
{
	my ( $data , $sFile , $root , $force ) = @_;
	my $bName = $sFile;
	$bName =~ s/\.cdf$//;
	$bName =~ s/^$root\///;
	
	my $fData = $data->{$bName};
	my $cFile = "$root/$bName.c";
	if ( -f $cFile ) {
		if ( $force ) {
			move( $cFile , "$cFile.bak" );
		} else {
			print STDERR "$cFile exists and will not be overwritten. Use -f to force.\n";
			return;
		}
	}

	open( my $fh , ">" , "$cFile" )
		or die "Unable to create '$cFile'\n";
	print "Creating $cFile ...\n";

	# Generate header
	print $fh $fData->{fileComment} if defined $fData->{fileComment};
	print $fh <<"EOL";

/** \@file
 * \@brief $fData->{class} class implementation
 * \@note Initially generated from $sFile
 */

#include "$bName.h"


EOL

	# Method declarations - only virtual and private methods
	my $hasOne = 0;
	foreach my $methName ( @{ $fData->{order}{methods} } ) {
		my $method = $fData->{methods}{$methName};
		next if ( $method->{inline} || $method->{abstract} );
		next unless ( $method->{virtual} || !$method->{public} );

		if ( !$hasOne ) {
			print $fh "/* Declarations of method implementations */\n";
			$hasOne = 1;
		}

		print $fh "static " . $method->{retType} . " _$fData->{class}_$methName(";

		my @params = @{ $method->{params} };
		unshift @params, { 'object' => $method->{public} ? $method->{onClass} : $fData->{class} }
			unless $method->{static};

		my @pStr = ( );
		foreach my $mParam ( @params ) {
			my $pName = ( keys %$mParam )[ 0 ];
			my $pType = ( values %$mParam )[ 0 ];
			push @pStr , " $pType $pName";
		}
		print $fh join( ", " , @pStr ) . " );\n";
	}

	# Declare functions for property validation / onChange / free / copy
	$hasOne = 0;
	my %propHandlers = ( );
	foreach my $propName ( @{ $fData->{order}{props} } ) {
		my $prop = $fData->{props}{$propName};
		next unless ( $prop->{pType} eq 'OOL_PTYPE_CUSTOM' || $prop->{validator} || $prop->{change} );

		if ( !$hasOne ) {
			print $fh "\n/* Declarations of property handlers */\n";
			$hasOne = 1;
		}

		my %handlers = ();
		$handlers{PV} = [ "qboolean" , "const void * value" , "Validator" ]
			if $prop->{validator};
		$handlers{PC} = [ "void" , "OOL_Object object , const char * name" , "Change handler" ]
			if $prop->{change};
		if ( $prop->{pType} eq 'OOL_PTYPE_CUSTOM' ) {
			$handlers{CPC} = [ "void" , "const void * source , void * dest" , "Copy function" ];
			$handlers{CPF} = [ "void" , "void * property" , "Destructor" ]
				if $prop->{free};
		}
		$propHandlers{$propName} = { %handlers };

		foreach my $hType ( keys %handlers ) {
			my $handler = $handlers{$hType};
			print $fh "static " . $handler->[0] . " _$hType\_$fData->{class}_$propName(" . $handler->[1] . ");\n";
		}
	}

	# Define static private fields
	$hasOne = 0;
	foreach my $fieldName ( @{ $fData->{order}{cfields} } ) {
		my $field = $fData->{cfields}{$fieldName};
		next if $field->{public};

		if ( $field->{comment} ne '' ) {
			my $comment = $field->{comment};
			$comment =~ s/\n\t+/\n/gs;
			print $fh "\n$comment\n";
		}

		print $fh "static $field->{type} _$fData->{class}_$fieldName";
		if ( @{$field->{arrayDims}} ) {
			print $fh '[' . join('][' , @{$field->{arrayDims}} ) . ']';
		}
		print $fh ";\n";
	}

	# Class structure variables and class-defining function
	print $fh <<"EOL";


/** \@brief $fData->{class} class definition */
static struct $fData->{class}_cs _$fData->{class}_class;
/** \@brief $fData->{class} class definition pointer */
static OOL_Class _$fData->{class}_cptr = NULL;


/* Class-defining function; documentation in $bName.h */
OOL_Class $fData->{class}__Class( )
{
\tif( _$fData->{class}_cptr != NULL ) {
\t\treturn _$fData->{class}_cptr;
\t}

\t_$fData->{class}_cptr = (OOL_Class) & _$fData->{class}_class;
\tOOL_InitClassBasics( _$fData->{class}_class , $fData->{class} , $fData->{parent} );
EOL

	# Set method function pointers
	$hasOne = 0;
	foreach my $methName ( @{ $fData->{order}{methods} } ) {
		my $method = $fData->{methods}{$methName};
		next unless $method->{virtual};

		if ( !$hasOne ) {
			print $fh "\n\t// Set virtual method pointers\n";
			$hasOne = 1;
		}

		if ( $method->{abstract} ) {
			print $fh "\t_$fData->{class}_class.$methName = NULL;\n";
		} else {
			print $fh "\tOOL_SetMethod( $fData->{class} , $method->{onClass} , $methName );\n";
		}
	}

	# Initialise properties
	$hasOne = 0;
	foreach my $propName ( @{ $fData->{order}{props} } ) {
		my $prop = $fData->{props}{$propName};

		if ( ! $hasOne ) {
			print $fh "\n\t// Register properties\n";
			$hasOne = 1;
		}

		if ( $prop->{pType} ne 'OOL_PTYPE_CUSTOM' && !( $prop->{validator} || $prop->{change} ) ) {
			# Simple property
			print $fh "\tOOL_Class_AddProperty( _$fData->{class}_cptr , struct $fData->{class}_s , $propName , $prop->{pType} );\n";
		} else {
			# Full property definition
			print $fh "\tOOL_Class_AddPropertyEx( _$fData->{class}_cptr , \"$propName\" ,\n"
				. "\t\tPTR_FieldOffset( struct $fData->{class}_s , $propName ) , $prop->{pType} ,\n"
				. "\t\tPTR_FieldSize( struct $fData->{class}_s , $propName ) ,\n"
				. "\t\t" . ( $prop->{validator} ? "_PV_$fData->{class}_$propName" : "NULL" ) . " ,\n"
				. "\t\t" . ( $prop->{change} ? "_PC_$fData->{class}_$propName" : "NULL" ) . " ,\n"
				. "\t\t" . ( ( $prop->{pType} eq 'OOL_PTYPE_CUSTOM' ) ? "_CPC_$fData->{class}_$propName" : "NULL" ) . " ,\n"
				. "\t\t" . ( $prop->{free} ? "_CPF_$fData->{class}_$propName" : "NULL" ) . " );\n";
		}
	}

	# Initiallise class fields
	$hasOne = 0;
	foreach my $fieldName ( @{ $fData->{order}{cfields} } ) {
		my $field = $fData->{cfields}{$fieldName};

		if ( !$hasOne ) {
			print $fh "\n\t// Initialise static fields\n";
			$hasOne = 1;
		}

		print $fh "\t_$fData->{class}_" . ($field->{public} ? "class." : "" ) . "$fieldName = $field->{init};\n";
	}

	# End of class-defining function
	print $fh "\n\treturn _$fData->{class}_cptr;\n}\n";

	# Method bodies
	foreach my $methName ( @{ $fData->{order}{methods} } ) {
		my $method = $fData->{methods}{$methName};
		next if ( $method->{inline} || $method->{abstract} );

		my $mType = $method->{public} ? "public" : "private";
		$mType .= $method->{static} ? " static" : "";
		$mType .= $method->{virtual} ? " polymorphic" : "";
		$mType .= " method";
		if ( $method->{virtual} && $method->{onClass} ne $fData->{class} ) {
			$mType .= " overriding <b>$method->{onClass}\:\:$methName</b>";
		}

		if ( ( ! $method->{public} || ( $method->{virtual} && $method->{onClass} ne $fData->{class} ) ) && $method->{comment} ne '' ) {
			my $comment = $method->{comment};
			$comment =~ s/\n\t+/\n/gs;
			print $fh "\n\n$comment\n";
		} elsif ( ! ( $method->{public} && $method->{static} ) ) {
			print $fh <<"EOL";


/** \@brief <b>$fData->{class}\:\:$methName</b> method implementation
 * \@note $mType
 *
 * XXX: write implementation documentation here
 */
EOL
		} else {
			print $fh "\n\n/* Documentation in $bName.h */\n";
		}

		my ( $prefix , $mPrefix );
		if ( $method->{public} && !$method->{virtual} ) {
			$prefix = "";
			$mPrefix = "";
		} else {
			$prefix = "static ";
			$mPrefix = "_";
		}
		print $fh $prefix . $method->{retType} . " $mPrefix$fData->{class}_$methName(";

		my @params = @{ $method->{params} };
		unshift @params, { 'object' => $method->{public} ? $method->{onClass} : $fData->{class} }
			unless $method->{static};

		my @pStr = ( );
		my @pnStr = ( );
		foreach my $mParam ( @params ) {
			my $pName = ( keys %$mParam )[ 0 ];
			my $pType = ( values %$mParam )[ 0 ];
			push @pStr , " $pType $pName";
			push @pnStr , " $pName";
		}
		print $fh join( ", " , @pStr ) . " )\n{\n";

		if ( defined( $method->{super} ) && !$method->{super} ) {
			print $fh "\tOOL_ClassCast( $fData->{parent}__Class( ) , $method->{onClass} )->$methName("
				. join( ", " , @pnStr ) . " );\n";
		}

		print $fh "\t// XXX: write method code here\n";

		if ( defined( $method->{super} ) && $method->{super} ) {
			print $fh "\tOOL_ClassCast( $fData->{parent}__Class( ) , $method->{onClass} )->$methName("
				. join( ", " , @pnStr ) . " );\n";
		}

		if ( $method->{retType} ne 'void' ) {
			print $fh "\treturn ($method->{retType})0; // XXX: change this\n";
		}

		print $fh "}\n";
	}

	# Property handler bodies
	foreach my $propName ( keys %propHandlers ) {
		my %prop = %{ $propHandlers{$propName} };
		foreach my $phType ( keys %prop ) {
			my @phData = @{ $prop{$phType} };
			print $fh <<"EOL";


/** \@brief $phData[2] for property <b>$fData->{class}\:\:$propName</b>
 *
 * XXX: documentation
 */
static $phData[0] _$phType\_$fData->{class}_$propName( $phData[1] )
{
	// XXX: implement handler
EOL
			if ( $phData[0] ne 'void' ) {
				print $fh "\treturn ($phData[0])0; // XXX: change this\n";
			}
			print $fh "}\n";
		}
	}

	close $fh;
}


###
# generate( $config )
#
# Callback for the -g command ; we need to parse the definitions then generate
# C code.
###
sub generate
{
	my $config = shift;
	my $data = {
		'qcommon/objects' => {
			class	=> 'OOL_Object' ,
			auto	=> 1 ,
			members	=> {
				PrepareInstance => 'methods' ,
				Initialise	=> 'methods' ,
				SetProperty	=> 'methods' ,
				Destroy		=> 'methods' ,
			} ,
			methods	=> {
				PrepareInstance	=> {
					super	=> 0 ,
					virtual	=> 1 ,
					retType	=> 'void' ,
					public	=> 1 ,
				} ,
				Initialise	=> {
					super	=> 0 ,
					virtual	=> 1 ,
					retType	=> 'void' ,
					public	=> 1 ,
				} ,
				SetProperty	=> {
					virtual	=> 1 ,
					params	=> [
						{ call_handler => 'qboolean' } ,
						{ name => 'const char *' } ,
						{ value => 'va_list' } ,
					] ,
					retType	=> 'void' ,
					public	=> 1 ,
				} ,
				Destroy		=> {
					virtual	=> 1 ,
					super	=> 1 ,
					retType	=> 'void' ,
					public	=> 1 ,
				} ,
			} ,
			props	=> { } ,
			fields	=> { } ,
			cfields	=> { } ,
		}
	};

	parseDefinitionFile( $data , $config->{target}[0] , $config->{root} );
	generateCHeader( $data , $config->{target}[0] , $config->{root} , $config->{force} );
	generateCSource( $data , $config->{target}[0] , $config->{root} , $config->{force} );
}



###
# Main program
#
# Parse command line arguments, then execute whatever action is necessary.
###
my $config = getCommandLine @ARGV;
&{ $config->{action} }( $config );
