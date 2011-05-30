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
/** @file
 * @brief @link qcommon_objects Object-oriented layer @endlink implementation
 */

#include "qcommon/objects.h"


static struct _OOL_property_s * _OOL_FindProperty( OOL_Class class ,
					const char * name );
static void _OOL_Object_EmptyConstructor( OOL_Object object );
static void _OOL_Object_SetProperty( OOL_Object object , qboolean call_handler ,
				const char * property , va_list value );
static void _OOL_Object_Destroy( OOL_Object object );


/** @brief Maximal length of a property's name */
#define MAX_PROPERTY_NAME_LENGTH 63


/** @brief Tag for memory allocation used by objects */
#define TAG_OBJECT 572

/** @brief Tag for memory allocation used by object properties */
#define TAG_PROPERTY 573

/** @brief Macro that allocates memory for an object
 *
 * @param X the amount of memory to allocate
 * @return the address of the allocated area
 */
#define OBJ_ALLOC(X) Z_TagMalloc((X),TAG_OBJECT)

/** @brief Macro that allocates memory for an object property
 *
 * @param X the amount of memory to allocate
 * @return the address of the allocated area
 */
#define PROP_ALLOC(X) Z_TagMalloc((X),TAG_PROPERTY)



/** @brief Property definition structure
 *
 * This structure is used by property tables to store information regarding a
 * property of objects that belong to a class.
 */
struct _OOL_property_s
{
	/** @brief Name of the property */
	char name[ MAX_PROPERTY_NAME_LENGTH + 1 ];

	/** @brief Location of the instance field
	 *
	 * A property always corresponds to an instance field. This field
	 * indicates the offset of the field within the instance structure.
	 */
	ptrdiff_t offset;

	/** @brief Size of the property
	 *
	 * This field's meaning differs depending on the type of property.
	 * It may indicate an actual size in bytes, or a maximal length. It
	 * may also be unused.
	 */
	size_t size;

	/** @brief Type of the property
	 *
	 * This field indicates the type of the property, therefore
	 * determining how it will be copied to the instance.
	 */
	unsigned int type;

	/** @brief Validator */
	OOL_validator_func_t validator;

	/** @brief Custom copy handler */
	OOL_copy_handler_func_t copy_handler;

	/** @brief Custom destructor */
	OOL_custom_destroy_func_t custom_free;

	/** @brief Change handler */
	OOL_change_handler_func_t change_handler;
};


/** @brief The #OOL_Object class definition */
static struct OOL_Object_cs _OOL_Object_class;

/** @brief Pointer to the #OOL_Object class definition */
static OOL_Class _OOL_Object_cptr = NULL;



/* Documented in objects.h */
OOL_Class OOL_Object__Class( )
{
	if ( _OOL_Object_cptr ) {
		return _OOL_Object_cptr;
	}

	_OOL_Object_class.parent = NULL;
	_OOL_Object_class.size = sizeof( struct OOL_Object_s );
	_OOL_Object_class.properties = NULL;
	_OOL_Object_class.PrepareInstance = _OOL_Object_EmptyConstructor;
	_OOL_Object_class.Initialise = _OOL_Object_EmptyConstructor;
	_OOL_Object_class.SetProperty = _OOL_Object_SetProperty;
	_OOL_Object_class.Destroy = _OOL_Object_Destroy;

	return ( _OOL_Object_cptr = &_OOL_Object_class );
}



/* Documented in objects.h */
OOL_Object OOL_Object_CreateEx( OOL_Class class , ... )
{
	OOL_Object object;
	va_list args;
	const char * p_name;

	assert( class != NULL );

	// Allocate and prepare the object
	object = OBJ_ALLOC( class->size );
	assert( object != NULL );
	memset( object , 0 , class->size );
	object->class = class;
	class->PrepareInstance( object );

	// Set properties
	va_start( args , class );
	p_name = va_arg( args , const char * );
	while ( p_name != NULL ) {
		class->SetProperty( object , false , p_name , args );
		p_name = va_arg( args , const char * );
	}

	// Finish initialisation
	class->Initialise( object );
	return object;
}



/* Documented in objects.h */
void OOL_Class_AddPropertyEx( OOL_Class class , const char * name ,
		ptrdiff_t offset , unsigned int type , size_t size ,
		OOL_validator_func_t validator ,
		OOL_copy_handler_func_t custom_copy ,
		OOL_custom_destroy_func_t custom_free ,
		OOL_change_handler_func_t on_change )
{
	struct _OOL_property_s p_record;

	assert( class->parent != NULL );
	assert( offset >= class->parent->size );
	assert( offset + size <= class->size );
	assert( size > 0 );
	assert( _OOL_FindProperty( class , name ) == NULL );
	assert( type != OOL_PTYPE_CUSTOM
		|| ( type == OOL_PTYPE_CUSTOM && custom_copy != NULL ) );

	// Create the table if it does not exist
	if ( class->properties == NULL ) {
		class->properties = HT_Create( 23 ,
			HT_FLAG_INTABLE | HT_FLAG_CASE ,
			sizeof( struct _OOL_property_s ) ,
			PTR_FieldOffset( struct _OOL_property_s , name ) ,
			MAX_PROPERTY_NAME_LENGTH + 1 );
	}

	// Initialise the record
	Q_strncpyz2( p_record.name , name , MAX_PROPERTY_NAME_LENGTH + 1 );
	p_record.offset = offset;
	p_record.size = size;
	p_record.type = type;
	p_record.validator = validator;
	p_record.copy_handler = ( type == OOL_PTYPE_CUSTOM ? custom_copy : NULL );
	p_record.custom_free = ( type == OOL_PTYPE_CUSTOM ? custom_free : NULL );
	p_record.change_handler = on_change;

	// Add it to the table
	HT_PutItem( class->properties , &p_record , false );
}


/** @brief Find a class property
 *
 * Look for a property by going through the class and its ancestors
 * recursively.
 *
 * @param class the class
 * @param name the property's name
 *
 * @return the property's record or NULL if there was no such property
 */
static struct _OOL_property_s * _OOL_FindProperty( OOL_Class class ,
					const char * name )
{
	struct _OOL_property_s * p_record;
	do {
		if ( class->properties != NULL ) {
			p_record = HT_GetItem( class->properties , name , NULL );
		} else {
			p_record = NULL;
		}
		class = class->parent;
	} while ( class != NULL && p_record == NULL );
	return p_record;
}



/** @brief Empty constructor implementation
 *
 * This empty function is used for both OOL_Object_cs::PrepareInstance and
 * OOL_Object_cs::instance, as the constructors in the base class do not need
 * to do anything.
 *
 * @param object the object being initialised
 */
static void _OOL_Object_EmptyConstructor( OOL_Object object )
{
	/*PURPOSEDLY EMPTY*/
}


/** @brief Property setter implementation
 *
 * This function implements the property setter method (referenced by
 * OOL_Object_cs::SetProperty).
 *
 * @param object the object being updated
 * @param call_handler whether the property's change handler should be called
 * @param property the name of the property to update
 * @param value variadic argument list starting with the property's new value
 */
static void _OOL_Object_SetProperty( OOL_Object object , qboolean call_handler ,
				const char * property , va_list value )
{
	struct _OOL_property_s * p_record;
	void * obj_field;

	/** Attempt to find the property */
	p_record = _OOL_FindProperty( object->class , property );
	assert( p_record != NULL );

	/** Compute the property's field address. */
	obj_field = ( (char *) object ) + p_record->offset;

	/** Set the property depending on its type */
	if ( p_record->type <= OOL_PTYPE_WORD ) {
		// Integers
		unsigned int val = va_arg( value , unsigned int );
		if ( p_record->validator && ! p_record->validator( &val ) ) {
			return;
		}
		switch ( p_record->type ) {
			case OOL_PTYPE_BYTE:
				*( (unsigned char *) obj_field ) = val;
				break;
			case OOL_PTYPE_HALFWORD:
				*( (unsigned short int *) obj_field ) = val;
				break;
			case OOL_PTYPE_WORD:
				*( (unsigned int *) obj_field ) = val;
				break;
		}

	} else if ( p_record->type <= OOL_PTYPE_DOUBLE ) {
		// Real numbers
		double val = va_arg( value , double );
		if ( p_record->validator && ! p_record->validator( &val ) ) {
			return;
		}
		if ( p_record->type == OOL_PTYPE_DOUBLE ) {
			*( (double *) obj_field ) = val;
		} else {
			*( (float *) obj_field ) = val;
		}

	} else {
		// Various types of pointers
		void * ptr = va_arg( value , void * );
		if ( p_record->validator && ! p_record->validator( ptr ) ) {
			return;
		}

		assert( p_record->type <= OOL_PTYPE_CUSTOM );
		switch ( p_record->type ) {
			case OOL_PTYPE_POINTER:
				// Pointer value
				*( (void **) obj_field ) = ptr;
				break;

			case OOL_PTYPE_OBJECT:
				// Object pointer
				assert( ptr != (void *) object );
				if ( *( (void **) obj_field ) != NULL ) {
					OOL_Object_Destroy( *( (OOL_Object *) obj_field ) );
				}
				*( (void **) obj_field ) = ptr;
				break;

			case OOL_PTYPE_DIRECT_STRING:
				// In-structure string
				assert( ptr != NULL );
				Q_strncpyz2( (char *) obj_field , (char *) ptr ,
					p_record->size );
				break;

			case OOL_PTYPE_ALLOC_STRING:
				// Allocated string
				if ( *( (void **) obj_field ) != NULL ) {
					Z_Free( *( (void **) obj_field ) );
				}
				if ( ptr == NULL ) {
					*( (void **) obj_field ) = NULL;
				} else {
					*( (void **) obj_field ) = PROP_ALLOC( strlen( (char *) ptr ) );
					strcpy( *( (char **) obj_field ) , (char *) ptr );
				}
				break;

			case OOL_PTYPE_DIRECT_STRUCT:
				// In-structure structure
				assert( ptr != NULL );
				memcpy( obj_field , ptr , p_record->size );
				break;

			case OOL_PTYPE_ALLOC_STRUCT:
				// Allocated structure
				if ( ptr == NULL ) {
					if ( *( (void **) obj_field ) != NULL ) {
						Z_Free( *( (void **) obj_field ) );
						*( (void **) obj_field ) = NULL;
					}
				} else {
					if ( *( (void **) obj_field ) == NULL ) {
						*( (void **) obj_field ) = PROP_ALLOC( p_record->size );
					}
					memcpy( *( (void **) obj_field ) , ptr , p_record->size );
				}
				break;

			case OOL_PTYPE_CUSTOM:
				// Custom field
				assert( p_record->copy_handler != NULL );
				p_record->copy_handler( ptr , obj_field );
				break;
		}
	}

	/** Call the change handler if there is one and it hasn't been
	 * disabled.
	 */
	if ( call_handler && p_record->change_handler ) {
		p_record->change_handler( object , property );
	}
}


/** @brief Property destructor
 *
 * This function is used by #_OOL_Object_Destroy when an object's properties
 * are being destroyed. It will handle the destruction of individual
 * properties.
 *
 * @param item pointer to the property's record
 * @param extra pointer to the object
 *
 * @returns true as all properties need to be examined
 */
static qboolean _OOL_Object_DestroyProperty( void * item , void * extra )
{
	struct _OOL_property_s * property = item;
	void ** obj_field;

	obj_field = (void **)( ( (char *) extra ) + property->offset );

	if ( property->type == OOL_PTYPE_OBJECT && *obj_field ) {
		OOL_Object_Destroy( (OOL_Object) *obj_field );
	} else if ( ( property->type == OOL_PTYPE_ALLOC_STRING
			|| property->type == OOL_PTYPE_ALLOC_STRUCT )
			&& *obj_field ) {
		Z_Free( *obj_field );
	} else if ( property->type == OOL_PTYPE_CUSTOM
			&& property->custom_free ) {
		property->custom_free( *obj_field );
	}

	return true;
}


/** @brief Destructor implementation
 *
 * The base class destructor implementation goes through the object's class
 * (and class ancestors), freeing memory used by properties which may be
 * allocated automatically. It also calls the destructor of object-like
 * properties.
 */
static void _OOL_Object_Destroy( OOL_Object object )
{
	OOL_Class class = object->class;
	while ( class != NULL ) {
		if ( class->properties ) {
			HT_Apply( class->properties , _OOL_Object_DestroyProperty , object );
		}
		class = class->parent;
	}
}
