//========= Copyright Valve Corporation ============//
// See https://github.com/ValveSoftware/openvr/blob/master/src/vrcommon/pathtools_public.h

#ifndef __HOBO_VR_PATH_HELPERS
#define __HOBO_VR_PATH_HELPERS

#include <string>

namespace hobovr {

inline char Path_GetSlash() {
#if defined(WIN)
	return '\\';
#else
	return '/';
#endif
}
inline std::string Path_GetExtension( const std::string & sPath ) {
	for ( std::string::const_reverse_iterator i = sPath.rbegin(); i != sPath.rend(); i++ )
	{
		if ( *i == '.' )
		{
			return std::string( i.base(), sPath.end() );
		}

		// if we find a slash there is no extension
		if ( *i == '\\' || *i == '/' )
			break;
	}

	// we didn't find an extension
	return "";
}
inline std::string Path_StripFilename( const std::string & sPath, char slash = 0 ) {
	if( slash == 0 )
		slash = Path_GetSlash();

	std::string::size_type n = sPath.find_last_of( slash );
	if( n == std::string::npos )
		return sPath;
	else
		return std::string( sPath.begin(), sPath.begin() + n );
}
inline std::string Path_Join( const std::string & first, const std::string & second, char slash = 0 ) {
	if( slash == 0 )
		slash = Path_GetSlash();

	// only insert a slash if we don't already have one
	std::string::size_type nLen = first.length();
	if( !nLen )
		return second;
#if defined(WIN)
	if( first.back() == '\\' || first.back() == '/' )
	    nLen--;
#else
	char last_char = first[first.length()-1];
	if (last_char == '\\' || last_char == '/')
	    nLen--;
#endif

	return first.substr( 0, nLen ) + std::string( 1, slash ) + second;
}

}

#endif // #ifndef __HOBO_VR_PATH_HELPERS