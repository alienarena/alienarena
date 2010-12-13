/*
Copyright (C) 2010 COR Entertainment

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/** \file
 * \brief \link qcommon_objects Object-oriented layer \endlink declarations
 */

#ifndef __H_QCOMMON_OBJECTS
#define __H_QCOMMON_OBJECTS

#include "qcommon/qcommon.h"
#include "qcommon/htable.h"
#include "qcommon/ptrtools.h"


/** \defgroup qcommon_objects Object-oriented layer
 * \ingroup qcommon
 *
 * The object-oriented layer provides a foundation for using some of the OO
 * concepts from AlienArena's C code:
 * - encapsulation, to a limited degree (there no private class members),
 * - inheritance,
 * - polymorphism,
 * - limited reflection (only usable to set properties).
 *
 * In order to do that, the object-oriented layer provides a few convenience
 * macros and functions, as well as a base class.
 *
 *
 * \section qcommon_objects_internals How the OO layer works
 *
 * The object-oriented layer needs to manipulate two different types of
 * structures: one that represents classes themselves, and one that represents
 * instances of classes. A function that initialises and return a variable for
 * a class structure is needed for each class. Finally, four commonly used
 * methods are defined in the base class.
 *
 * \subsection qcommon_objects_internals_cstruct Class structure
 *
 * The <b>class structure</b> for a given class must always start by a field
 * which encapsulates the class structure of its parent. Because a class might
 * have a different instance size, because it will not have the same
 * properties, and because it might override methods, the contents of the
 * parent class' structure must be copied and modified.
 *
 * In addition, the base class, defined by the #OOL_Object_cs structure,
 * provides a pointer to the structure which defines the parent class. This
 * allows the type of an instance to be investigated at runtime (through
 * #OOL_IsInstance and, if debugging is enabled, whenever a cast is made).
 * It is also necessary when defining or setting properties, as a property
 * might be defined in a class or in any one of its parents.
 *
 * Finally, the class structure should contain function pointer fields for all
 * new methods defined by a given class, as well as all class-scoped data.
 * 
 * \subsection qcommon_objects_internals_istruct Instance structure
 *
 * The <b>instance structure</b> for a given class should always start with a
 * field that encapsulates the parent class' instance structure. Other fields,
 * corresponding to instance-scoped data, might follow that initial field.
 *
 * The base class' instance structure, #OOL_Object_s, contains a single field:
 * a pointer to the class structure which defines the class of the object. This
 * allows the type of an instance to be evaluated at run-time, providing
 * support for limited introspection through #OOL_IsInstance, type checks when
 * casting through #OOL_Cast if debugging is enabled, and of course access to
 * the class' methods and data.
 *
 * \subsection qcommon_objects_internals_cdfuncs Class-defining functions
 *
 * Class structures themselves are insufficient on their own; a (hopefully
 * unique) variable using that structure must be defined somewhere. However,
 * statically defining such a variable is next to impossible.
 *
 * A class-defining function will make sure that the variable containing the
 * class structure has been properly initialised, and initialise it if that is
 * necessary. It will then return a pointer to that variable (cast to the
 * #OOL_Class pointer type).
 *
 * \subsection qcommon_objects_internals_methods Common methods
 *
 * The base class provides four methods. Two of these methods are required by
 * the object creation function, one is used both at object creation and when
 * a property is changed, and the last one is needed to properly destroy
 * objects.
 *
 * When an object is created using #OOL_Object_CreateEx or one of its variants,
 * the memory for the object is allocated, its type field is set, and the rest
 * of the instance's data is set to zero. The class' <b>PrepareInstance</b>
 * method is then called; this method should allocate any memory the object
 * always requires, and set default values for properties. Once it returns,
 * the initial properties are set from the arguments. Then the class'
 * <b>Initialise</b> method is called. Both methods should be overridden as
 * necessary to initialise objects of various types.
 *
 * The base class' <b>SetProperty</b> method is used to set properties on an
 * instance. It is used during object initialisation, if there are properties
 * to set; when that happens, the property's change handler is not called.
 * However, the method is also called by #OOL_Object_Set, in which case the
 * change handler <i>is</i> called. This method should not be overridden.
 *
 * Finally, a class' <b>Destroy</b> method is called before the object's
 * memory is freed. It should be overridden by all classes which allocate
 * resources in order to free these resources. However, it is absolutely
 * necessary to call the parent class' <b>Destroy</b> method in any
 * overridden version.
 *
 * \note The base class' <b>Destroy</b> method will free any memory used by
 * automatically allocated properties (#OOL_PTYPE_ALLOC_STRING and
 * #OOL_PTYPE_ALLOC_STRUCT), destroy object properties, and call custom
 * destruction handlers for #OOL_PTYPE_CUSTOM properties which define such
 * a handler.
 *
 *
 * \section qcommon_objects_defining Defining a class
 *
 * To define a class, it is necessary to:
 * - define its two public structures and declare its defining function,
 * usually in a header;
 * - write the class defining function and the code for the class' methods.
 * In addition, it is recommended to write a set of inline functions that
 * will call any new method the class defines.
 *
 * \subsection qcommon_objects_defining_names Naming conventions
 *
 * Because of the OO layer's reliance on various macros, it is necessary for
 * object-oriented code to follow the following naming conventions when
 * a class is being defined.
 *
 * - A class' structure must always be named <b><i>classname</i>_cs</b>.
 * - A class' instance structure must always be named
 * <b><i>classname</i>_s</b>, and a typedef for a pointer to this structure,
 * named <b><i>classname</i></b>, must be provided.
 * - The class' defining function must be named <b><i>classname</i>__Class</b>
 * and it must return an #OOL_Class.
 * - It is recommended (albeit not necessary) to create inline functions that
 * provide direct access to the class' methods. These functions should be
 * named <b><i>classname</i>_<i>methodname</i></b>.
 *
 * \subsection qcommon_objects_defining_structures Class and instance structures
 *
 * Even if the class does not define any new method, it is always necessary to
 * define a class structure - not doing so might break the various macros that
 * manipulate classes. Any new method the class provides should be added as a
 * function pointer to some function, with the first argument being an object
 * pointer of the appropriate type.
 *
 * The first field of both the class and instance structures must be the
 * corresponding structure from the parent class; the name of this field is
 * irrelevant, but it has to be present.
 *
 * The instance structure must always be defined as well, even if it does not
 * include any new data.
 *
 * Here is an example of class and instance structures definition:
 *
 * \code
 * // Forward-declaration of the instance structure and pointer type
 * struct TestObject_s;
 * typedef struct TestObject_s * TestObject;
 *
 * // This is the class structure:
 * struct TestObject_cs
 * {
 *     // Parent class' class structure
 *     struct OOL_Object_cs p;
 *
 *     // This class defines a "Print" method.
 *     void (* Print)( TestObject object );
 *
 *     // And this is class-scoped data
 *     int class_integer;
 * };
 *
 * // The instance structure, with a single string pointer field:
 * struct TestObject_s
 * {
 *     // Parent class' instance structure
 *     struct OOL_Object_s p;
 *
 *     // A string pointer field
 *     char * str_field;
 * };
 * \endcode
 *
 * \subsection qcommon_objects_defining_cdfunc Class-defining function
 *
 * The class-defining function is responsible for initialising the class'
 * definition through a variable using the class structure.
 *
 * Since the class-defining function is called quite frequently, however,
 * it should not initialise that variable every time. It is therefore
 * recommended to create two static variables: one that will contain the
 * class definition itself, and one that will be initially NULL and point to
 * the definition once it has been initialised.
 *
 * \code
 * // The definition itself
 * static struct TestObject_cs _TestObject_class;
 * // Pointer to the definition (will be set when the definition is initialised).
 * static struct TestObject_cs * _TestObject_cptr = NULL;
 * \endcode
 *
 * The first thing the defining function should do is check if the pointer
 * variable has been initialised. If it has, it should return its value
 * directly.
 *
 * \code
 * OOL_Class TestObject__Class( )
 * {
 *     if ( _TestObject_cptr ) {
 *         return (OOL_Class) _TestObject_cptr;
 *     }
 * \endcode
 *
 * If the check fails, the class needs to be initialised; the first step for
 * that is to copy the parent class' definition and to (re)set some of the
 * basic fields. The #OOL_InitClassBasics macro takes care of that.
 *
 * \code
 *     OOL_InitClassBasics( _TestObject_class , TestObject , OOL_Object );
 * \endcode
 *
 * New and overridden methods should then be installed, and class-scoped
 * values initialised:
 *
 * \code
 *     // In order for the code below to work, the functions implementing the
 *     // various methods should be named _TestObject_PrepareInstance,
 *     // _TestObject_Destroy and _TestObject_Print; they should have been
 *     // declared before TestObject__Class()'s definition.
 *
 *     // Override PrepareInstance and Destroy
 *     OOL_SetMethod( TestObject , OOL_Object , PrepareInstance );
 *     OOL_SetMethod( TestObject , OOL_Object , Destroy );
 *     // Set up the Print method
 *     OOL_SetMethod( TestObject , TestObject , Print );
 *     // Initialise class-scoped value
 *     _TestObject_class.class_integer = 0;
 * \endcode
 *
 * \note the #OOL_SetMethod macro is based on a few assumptions; in special
 * cases it might be preferrable to use #OOL_SetMethodEx. See both macros'
 * documentation for more information.
 *
 * Finally, the function should set the pointer variable and return it.
 *
 * \code
 *     _TestObject_cptr = &_TestObject_class;
 *     return (OOL_Class) _TestObject_cptr;
 * }
 * \endcode
 *
 * \subsection qcommon_objects_defining_inline Inline functions for method calls
 *
 * In order to call an object's methods, two casts are usually necessary:
 * - casting the object's class, in order to access the class structure in
 * which the method is defined;
 * - casting the object itself so it can be given to the method.
 *
 * In order to avoid having to do this directly in code that uses objects, it
 * is recommended to create inline functions in header files. Here is an
 * example of such a function:
 *
 * \code
 * 
 * static inline void TestObj_Print( OOL_Object object )
 * {
 *     OOL_GetClassAs( object , TestObject )->Print( OOL_Cast( object , TestObject ) );
 * }
 * \endcode
 *
 * \note Such an inline function only adds a performance cost if debugging is
 * enabled, as in any other case the typecast macros will only do actual
 * casts between pointers. When debugging is enabled, however, both macros will
 * ensure that the type is compatible.
 * 
 *
 * \section qcommon_objects_props Properties
 *
 * Properties provide a limited reflection system allowing instance fields to
 * be set in a generic manner. They allow automatic management of memory for
 * strings or structures; in addition, having a single function capable of
 * allocating and initialising any type of object wouldn't be possible without
 * them.
 *
 * \subsection qcommon_objects_props_internals Implementation
 *
 * The property mechanism is implemented at class level. It is based on an
 * optional hash table (which is only added to a class if it defines
 * properties). Each entry of this table provides information about the
 * property and the field it corresponds to. The following information is
 * part of a property's description:
 * - the <b>name</b> of the property,
 * - the <b>type</b> of the property, which determines how the values are
 * manipulated,
 * - the <b>size</b> and <b>offset</b> of the field in the class' instance
 * structure which is mapped to the property,
 * - an optional <b>change handler</b>, pointer to a function which is called
 * when the property is modified after the instance has been initialised,
 * - an optional <b>validator</b> which makes sure that a property's new value
 * is acceptable,
 * - in the case of properties with a #OOL_PTYPE_CUSTOM type, a mandatory
 * <b>copy handler</b> which copies the value of the property and an optional
 * <b>destructor</b> which is called by the class' destructor.
 *
 * A property must be declared in the class where the corresponding field
 * is defined. In addition, a property's name must be unique within a class'
 * ancestry.
 *
 * When a property is being set, its information will be looked up (first in
 * the class itself, then in its parent, etc...). If there is a validator,
 * it is called and, depending on the return value, the operation either
 * continues or is interrupted. The new value is then copied depending on
 * its type. Finally the change handler is called if it is defined (unless the
 * property is being set from the object creation function).
 *
 * \subsection qcommon_objects_props_types Types of properties
 *
 * - <b>Basic properties</b> -
 * Properties which correspond to integers, pointers or real numbers can be
 * defined using the corresponding property types (3 types for integers,
 * depending on the size; two types for real numbers, depending on the
 * precision; and one type for pointers).
 * When setting such a property, the new value is copied directly into the
 * instance structure for the object.
 *
 * - <b>String properties</b> -
 * Two different types of string properties are supported. It is possible to
 * have a string stored directly in the instance structure, using an array
 * of characters; this corresponds to the #OOL_PTYPE_DIRECT_STRING type.
 * However it is also possible to use dynamically-allocated strings; in this
 * case, the memory used to store the string will be managed automatically.
 * This is the #OOL_PTYPE_ALLOC_STRING property type.
 * In both cases, the contents of the string are copied; however, when the
 * string is dynamically allocated, it is also possible to set the property to
 * NULL (which will free any previously used memory).
 *
 * - <b>Structured properties</b> -
 * Structured properties are handled in the same way as string properties, the
 * main difference being that they have a fixed size. The corresponding
 * property types are #OOL_PTYPE_DIRECT_STRUCT for structures that are
 * directly stored inside the instance structure, and ##OOL_PTYPE_ALLOC_STRUCT
 * when only a pointer is stored. When setting such a property, a pointer to
 * the structure to copy should be passed.
 *
 * - <b>Object properties</b> -
 * It is possible to use objects as property values. The property code does not
 * perform any type-checking on such objects when the property is being
 * modified; this would have to be implemented through a validator. However,
 * objects set through properties will be deleted automatically when the
 * property is being modified or when the object containing it is destroyed.
 * It is also possible to set an object property to NULL.
 *
 * - <b>Custom properties</b> -
 * It is possible to define properties that are not managed by the property's
 * default code. This can be done by using the #OOL_PTYPE_CUSTOM type and
 * specifying a copy function for the property. In addition, if the property
 * requires specific handling when the object is being destroyed, a destruction
 * function can be specified.
 *
 * \subsection qcommon_objects_props_defining Defining properties
 *
 * While it is in theory possible to define properties at any time, their
 * registration should occur in a class' defining function. Properties can be
 * defined:
 * - using the #OOL_Class_AddProperty macro, which is the simplest way,
 * although the macro does not support adding custom functions,
 * - or using the #OOL_Class_AddPropertyEx function directly, which is a
 * little more complex but allows custom functions to be added.
 *
 * 
 * \section qcommon_objects_use Manipulating objects
 *
 * Objects that instantiate classes defined using the object-oriented layer
 * need to be created. Once that has been done, one can call their methods,
 * update their properties, and access their fields. Finally, when an object
 * is no longer required, it must be destroyed.
 *
 * \subsection qcommon_objects_use_create Creating and destroying objects
 *
 * A simple object that instantiates a class can be created using the
 * #OOL_Object_Create macro. This is the simplest form of object creation:
 * it will allocate the object's memory, clear it, then call both of the
 * class' initilisation methods.
 *
 * The #OOL_Object_CreateWith macro can be used to set some initial properties
 * after the object has been allocated and prepared, before its second
 * initialisation method is called. Properties need to be passed as pairs: a
 * string naming the property, followed by the property's value.
 *
 * \code
 * OOL_Object my_obj;
 * // Assuming the TestObject class has a string property named "str_prop"
 * my_obj = OOL_Object_CreateWith( TestObject , "str_prop" , "Hello, world!" );
 * \endcode
 *
 * Objects can be destroyed by calling the #OOL_Object_Destroy function.
 *
 * \subsection qcommon_objects_use_manip Manipulating objects
 *
 * The first element that can be used to manipulate an object is the methods
 * its class offers. These methods will usually be made available through
 * inline functions. It is also possible to bypass polymorphism on an object
 * by calling the method defined by an ancestor class directly. This is useful
 * when e.g. calling super methods from inside an object.
 *
 * \code
 * // Assuming TestChild is a child class of TestObject, and that TestObject
 * // has a "Print" method which is overridden in TestChild.
 * my_obj = OOL_Object_Create( TestChild );
 * // This will call *TestObject*'s Print method on my_obj.
 * OOL_GetClass( TestObject )->Print( my_obj );
 * \endcode
 *
 * \note More complex operations can be achieved through #OOL_ClassCast. For
 * example, TestChild's destructor might contain the following code in order
 * to call its parent's destructor:
 *
 * \code
 * OOL_ClassCast( TestObject__Class( ) , OOL_Object )->Destroy( self );
 * \endcode
 *
 * It is possible to set properties on an object after it has been initialised.
 * This can be done through the #OOL_Object_Set function; the property's new
 * value will be validated and copied to the object; its change handler will be
 * called, if there is one.
 *
 * \code
 * OOL_Object_Set( my_obj , "str_prop" , "Salut, le monde!" );
 * \endcode
 *
 * Finally, it is always possible to access instance fields directly; this can
 * be done using the #OOL_Field macro. While it is possible to set
 * property-controlled fields using the macro, it is not recommended.
 *
 * \code
 * OOL_Field( my_obj , TestObject , int_field ) = 42;
 * \endcode
 *
 */
/*@{*/


struct OOL_Object_s;


/** \brief Generic object type */
typedef struct OOL_Object_s * OOL_Object;


/** \brief Structure for the base class
 *
 * This structure represents the base class of all objects managed by the
 * \link qcommon_objects OO layer \endlink.
 *
 * It includes information about:
 * - the class' ancestor,
 * - the size of an instance,
 * - the table of properties which can be set at construction or through the
 * #OOL_Object_Set function,
 * - basic methods for instance management.
 */
struct OOL_Object_cs
{
	/** \brief Parent class
	 *
	 * With the exception of the OOL_Object class itself, all classes
	 * inherit some other class. This field points to the structure that
	 * defines the parent class.
	 */
	struct OOL_Object_cs * parent;

	/** \brief Reflexive property table
	 *
	 * This hash table stores details about the various properties which
	 * can be set through #OOL_Object_CreateEx or #OOL_Object_Set for the
	 * current class.
	 */
	hashtable_t properties;

	/** \brief Size of an object
	 *
	 * This field indicates the size of an object of the class the
	 * structure represents.
	 */
	size_t size;

	/** \brief Pre-initialisation method
	 *
	 * This method is called on an object that has just been allocated
	 * by #OOL_Object_CreateEx ; it should allocate all additional memory
	 * required for #OOL_Object_Set to function properly.
	 *
	 * \param object the newly allocated object
	 */
	void (* PrepareInstance)( OOL_Object object );

	/** \brief Initialisation method
	 *
	 * This method is called on an object that has been prepared and that
	 * has had its properties set. It performs the object's
	 * initialisation.
	 *
	 * \param object the object to initialise
	 */
	void (* Initialise)( OOL_Object object );

	/** \brief Property setter method
	 *
	 * This method uses the #properties table to set a property. It may
	 * call the property's handler if there is one and if it has been
	 * requested.
	 *
	 * This method is used by both #OOL_Object_CreateEx (with no handler
	 * calls) and by #OOL_Object_Set (with handler calls).
	 *
	 * \note If the requested property does not exist, this will either
	 * cause an assertion failure (if compiled with assertions enabled)
	 * or a plain old crash. In addition, the type of the property is
	 * excepted to match that of the parameter; if not, there's a high
	 * probability of crashing.
	 *
	 * \param object the object on which the property is to be changed
	 * \param call_handler whether the property's change handler should
	 * be called.
	 * \param property the name of the property
	 * \param va_list a list of variable arguments starting with the value
	 * of the property.
	 */
	void (* SetProperty)( OOL_Object object , qboolean call_handler ,
				const char * property , va_list value );

	/** \brief Destructor
	 *
	 * This method is called when the object is being destroyed through
	 * #OOL_Object_Destroy. It should free all resources allocated by
	 * the object.
	 *
	 * \param object the object being destroyed.
	 */
	void (* Destroy)( OOL_Object object );
};


/** \brief Generic class type */
typedef struct OOL_Object_cs * OOL_Class;


/** \brief Basic class definition initialisation
 *
 * This macro generates the code required to initialise the basic fields of a
 * class structure. It is meant to be used in a class' defining function.
 *
 * \param c_var the variable containing the class definition structure
 * \param c_name the name of the class
 * \param p_name the name of the parent class
 */
#define OOL_InitClassBasics(c_var,c_name,p_name) \
do { \
	OOL_Class parent = p_name##__Class( ); \
	memcpy( &(c_var) , parent , sizeof( struct c_name##_cs ) ); \
	( (OOL_Class) &(c_var) )->parent = parent; \
	( (OOL_Class) &(c_var) )->properties = NULL; \
	( (OOL_Class) &(c_var) )->size = sizeof( struct c_name##_s ); \
} while (0)


/** \brief Set a method's function pointer
 *
 * This macro can be used in defining functions to set function pointers for a
 * class' methods; it works for both new and overridden methods.
 *
 * \param c_var the variable containing the class definition structure
 * \param c_name the name of the class which contained the method to set
 * (it should be the class' own name for methods it defines, or the name of
 * any of its ancestors to override a method).
 * \param m_name the name of the method to set or override.
 * \param f_name the name of the method which implements the method.
 */
#define OOL_SetMethodEx(c_var,c_name,m_name,f_name) \
	( ( ( struct c_name##_cs * ) &(c_var) )->m_name = f_name )


/** \brief Simple method setter
 *
 * This macro is similar to #OOL_SetMethodEx, but it makes a few assumptions:
 * - that the class definition is stored in a variable called
 * <b>_<i>classname</i>_class</b>;
 * - that a function implementing a method (whether it is a new method or one
 * being overridden) is called <b>_<i>classname</i>_<i>methodname</i></b>.
 *
 * \param c_name the name of the class being defined
 * \param mc_name the name of the class in which the method is defined
 * \param m_name the name of the method.
 */
#define OOL_SetMethod(c_name,mc_name,m_name) \
	OOL_SetMethodEx( _##c_name##_class , mc_name , m_name , \
		_##c_name##_##m_name )


/** \brief Property validator function
 *
 * This type corresponds to functions that are used to validate a property's
 * new value before it is copied to the instance.
 *
 * \param value a pointer to the value
 */
typedef qboolean (* OOL_validator_func_t)( const void * value );


/** \brief Custom property copy function
 *
 * This type corresponds to functions that implement custom copying for
 * properties.
 *
 * \param source a pointer to the value
 * \param dest a pointer to the field to copy to
 */
typedef void (* OOL_copy_handler_func_t)( const void * source , void * dest );


/** \brief Custom property destructor
 *
 * This type corresponds to functions that implement custom destruction for
 * properties.
 *
 * \param property value of the property
 */
typedef void (* OOL_custom_destroy_func_t)( void * property );


/** \brief Property change handler
 *
 * This type corresponds to functions which can be used as property change
 * handlers registered through #OOL_Class_AddPropertyEx.
 *
 * \param object the instance on which a property is being changed
 * \param name the name of the property
 */
typedef void (* OOL_change_handler_func_t)( OOL_Object object ,
						const char * name );


/** \brief Property type - Single byte
 *
 * This property type is used for characters as well as signed or unsigned
 * bytes. The value taken from the parameters is copied directly to the
 * object's field.
 */
#define OOL_PTYPE_BYTE		0x00

/** \brief Property type - Two bytes
 *
 * This property type is used for two-byte integers (signed or unsigned). The
 * value taken from the parameters is copied directly to the object's field.
 */
#define OOL_PTYPE_HALFWORD	0x01

/** \brief Property type - Four bytes
 *
 * This property type is used for four-byte integers (signed or unsigned). The
 * value taken from the parameters is copied directly to the object's field.
 */
#define OOL_PTYPE_WORD		0x02


/** \brief Property type - Float
 *
 * This property type is used for floats. The value taken from the parameters
 * is copied directly to the object's field.
 */
#define OOL_PTYPE_FLOAT		0x03


/** \brief Property type - Double precision
 *
 * This property type is used for double-precision numbers. The value taken
 * from the parameters is copied directly to the object's field.
 */
#define OOL_PTYPE_DOUBLE	0x04


/** \brief Property type - Pointer
 *
 * This property type is used for pointers that must be copied directly to the
 * instance.
 */
#define OOL_PTYPE_POINTER	0x05


/** \brief Property type - Object pointer
 *
 * This property type is used for pointers to objects. They will be copied
 * directly to the instance; however, unlike #OOL_PTYPE_POINTER properties,
 * they will be destroyed along with the instance or when the property is
 * changed.
 */
#define OOL_PTYPE_OBJECT	0x06


/** \brief Property type - Direct string
 *
 * This property type is used for strings that must be copied from the pointer
 * found in the argument list directly into the instance.
 */
#define OOL_PTYPE_DIRECT_STRING	0x07


/** \brief Property type - Allocated string
 *
 * This property type is used for strings that must be copied from the pointer
 * found in the argument list to a specifically allocated area of memory.
 */
#define OOL_PTYPE_ALLOC_STRING	0x08


/** \brief Property type - Direct structure
 *
 * This property type is used for structures that must be copied from the
 * pointer found in the argument list directly into the instance.
 */
#define OOL_PTYPE_DIRECT_STRUCT	0x09


/** \brief Property type - Allocated structure
 *
 * This property type is used for structures that must be copied from the
 * pointer found in the argument list to a specifically allocated area of
 * memory.
 */
#define OOL_PTYPE_ALLOC_STRUCT	0x0a


/** \brief Property type - Custom property
 *
 * This property type is used when the type of the argument must go through
 * a custom copy function.
 */
#define OOL_PTYPE_CUSTOM	0x0b


/** \brief Add a property to a class
 *
 * This function, which should only be used during class initialisation, adds
 * a property to a class.
 *
 * First, if debugging is enabled, it will make sure that no property that has
 * the same name exists in the class hierarchy. It will also make sure that the
 * new property is inside the instance structure for the class, using its size
 * and its parent class' size.
 *
 * Then, if the class does not have a property table, it is created; finally,
 * the new record created and added to the table.
 *
 * \param class pointer to the class structure to which a property is to be
 * added
 * \param name the name of the new property
 * \param offset the offset of the property inside the class' instance
 * structure
 * \param type the type of the property being created
 * \param size the size of the property, if needed for the specified type
 * \param validator a validator function pointer (may be NULL)
 * \param custom_copy a custom copy function pointer (only used if the type
 * is #OOL_PTYPE_CUSTOM; must not be NULL in this case)
 * \param custom_free a custom destructor function pointer (only used if the
 * type is #OOL_PTYPE_CUSTOM; may be NULL)
 * \param on_change a change handler function pointer (may be NULL)
 */
void OOL_Class_AddPropertyEx( OOL_Class class , const char * name ,
		ptrdiff_t offset , unsigned int type , size_t size ,
		OOL_validator_func_t validator ,
		OOL_copy_handler_func_t custom_copy ,
		OOL_custom_destroy_func_t custom_free ,
		OOL_change_handler_func_t on_change );


/** \brief Add a property to a class, basic form
 *
 * This macro is meant to be used as a replacement for #OOL_Class_AddPropertyEx
 * in simple cases.
 *
 * It will add a property corresponding to a field of a structure, computing
 * the offset and size automatically, and using the field's name as the
 * property's name.
 *
 * \param class pointer to the class structure to which a property is to be
 * added
 * \param istruct the instance structure in which the field which will be
 * managed by the property can be found
 * \param field the name of the field which will be managed by the property
 * \param type the type of the property being created
 */
#define OOL_Class_AddProperty(class,istruct,field,type) \
	OOL_Class_AddPropertyEx( class , #field , \
		PTR_FieldOffset( istruct , field ) , type , \
		PTR_FieldSize( istruct , field ) , \
		NULL , NULL , NULL , NULL )



/** \brief Initialiser function for the Object class */
OOL_Class OOL_Object__Class( );


/** \brief Object structure */
struct OOL_Object_s
{
	/** \brief The object's class
	 *
	 * A pointer to the class structure for the class the current object
	 * is a direct instance of.
	 */
	struct OOL_Object_cs * class;
};


/** \brief Create an object
 *
 * This function allocates and initialises an instance of some type.
 *
 * It starts by allocating the required memory through #Z_Alloc, then prepares
 * the object using its \link OOL_Object_cs::PrepareInstance PrepareInstance
 * \endlink method.
 *
 * Once the object has been prepared, it will go through its variable
 * arguments and use them to set properties; these variable arguments are
 * paired: one is a string corresponding to the property's name, the second is
 * the property's value. Property setting completes when NULL is found where a
 * property should be.
 *
 * Once properties have been set, the object's initialisation is completed
 * using the \link OOL_Object_cs::Initialise Initialise \endlink method.
 *
 * \param class the object's class
 * \param ... properties to set on the object, followed by NULL
 *
 * \returns the new object
 */
OOL_Object OOL_Object_CreateEx( OOL_Class class , ... );


/** \brief Create an object with no properties
 *
 * This macro allows an object to be created with no properties. It is a
 * wrapper for #OOL_Object_CreateEx.
 *
 * \param class the name of the new class
 *
 * \returns the new object
 */
#define OOL_Object_Create(class) \
	OOL_Object_CreateEx( class##__Class( ) , NULL )

/** \brief Create an object with properties
 *
 * This macro allows an object to be created with a set of properties. It is a
 * wrapper for #OOL_Object_CreateEx, but unlike the function call, the list of
 * properties does not need to end with NULL.
 *
 * \param class the name of the new class
 * \param ... the property name/value pairs to use when initialising.
 *
 * \returns the new object
 */
#define OOL_Object_CreateWith(class,...) \
	OOL_Object_CreateEx( class##__Class( ) , __VA_ARGS__ , NULL )


/** \brief Set an object's property
 *
 * Set an object's property using its class' property table; if there is a
 * change handler associated with the property, it will be called.
 *
 * \note Notes regarding the property's name and value from
 * OOL_Object_cs::SetProperty apply here as well.
 *
 * \param object the object to update
 * \param property the name of the property
 * \param ... the value of the property
 */
static inline void OOL_Object_Set( OOL_Object object ,
					const char * property , ... )
{
	va_list arg;
	va_start( arg , property );
	object->class->SetProperty( object , true , property , arg );
	va_end( arg );
}


/** \brief Destroy an object
 *
 * Destroy the object by first calling its destructor then freeing the
 * memory it uses.
 *
 * \param object the object to destroy
 */
static inline void OOL_Object_Destroy( OOL_Object object )
{
	object->class->Destroy( object );
	Z_Free( object );
}



/** \brief Check if an object is an instance of some class
 *
 * \param object the object to check
 * \param class the class to use when checking
 *
 * \returns true if the object is an instance (direct or otherwise) of the
 * class, false if it isn't.
 */
static inline qboolean OOL_IsInstance( OOL_Object object , OOL_Class class )
{
	OOL_Class obj_class = object->class;
	while ( obj_class != NULL ) {
		if ( obj_class == class ) {
			return true;
		}
		obj_class = obj_class->parent;
	}
	return false;
}


/** \brief Check that a cast is valid
 *
 * This inline function is used when debugging is enabled to make sure that a
 * cast made through #OOL_Cast or #OOL_GetClassAs is valid.
 *
 * \note this function is only defined when debugging is enabled.
 *
 * \param object the object to cast
 * \param class the class the object is supposed to have
 *
 * \returns the object itself
 */
#ifndef NDEBUG
static inline OOL_Object _OOL_CheckCast( OOL_Object object , OOL_Class class )
{
	assert( OOL_IsInstance( object , class ) );
	return object;
}
#endif


/** \brief Object-oriented type cast
 *
 * This macro allows casting an object from a class to another; if debugging
 * is enabled, an additional check will be made to ensure that the object has
 * the correct type.
 *
 * \param object the object to cast
 * \param class the class the object is supposed to have
 *
 * \returns the typecasted object.
 */
#ifdef NDEBUG
#define OOL_Cast(object,class) ((class)(object))
#else
#define OOL_Cast(object,class) \
	( (class) _OOL_CheckCast( (OOL_Object)(object) , class##__Class( ) ) )
#endif


/** \brief Object-level class cast
 *
 * This macro allows accessing a class record of a specific type for an
 * object. If debugging is enabled, an additional check will be made to
 * ensure that the object has the correct type.
 *
 * \param object the object to cast
 * \param as_class the class to access
 *
 * \returns the typecasted object.
 */
#ifdef NDEBUG
#define OOL_GetClassAs(object,as_class) \
	((struct as_class##_cs *) (((OOL_Object)(object))->class))
#else
#define OOL_GetClassAs(object,as_class) \
	((struct as_class##_cs *) (_OOL_CheckCast( \
		(OOL_Object)(object) , as_class##__Class( ) )->class))
#endif


/** \brief Class cast
 *
 * This macro makes it possible to access fields and methods from an ancestor
 * class, given an initial class pointer.
 *
 * \note No checks are made, so the requested class must be compatible with
 * the class definition.
 *
 * \param c_def the class definition to access as another class
 * \param cast_to the name of the class to cast to.
 *
 * \returns the part of the original class definition that corresponds to the
 * specified class.
 */
#define OOL_ClassCast(c_def,cast_to) \
	( (struct cast_to##_cs *)( c_def ) )


/** \brief Class access with automatic cast
 *
 * This macro accesses a class definition, automatically casting it to its
 * own type.
 *
 * \param class the class to access
 *
 * \returns the class definition using its own class structure
 */
#define OOL_GetClass(class) \
	OOL_ClassCast( class##__Class( ) , class )


/** \brief Instance field access
 *
 * This macro makes the necessary cast to access an instance's field. It can
 * be used as both a l-value and a r-value.
 *
 * \param object the object to access
 * \param c_name the name of the class in which the field is defined
 * \param f_name the name of the field.
 */
#define OOL_Field(object,c_name,f_name) \
	( OOL_Cast(object,c_name)->f_name )


/*@}*/
#endif // __H_QCOMMON_OBJECTS
