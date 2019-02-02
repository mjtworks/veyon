/*
 * BuiltinDirectoryPlugin.cpp - implementation of BuiltinDirectoryPlugin class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QFile>

#include "BuiltinDirectoryConfigurationPage.h"
#include "BuiltinDirectory.h"
#include "BuiltinDirectoryPlugin.h"
#include "CommandLineIO.h"
#include "ConfigurationManager.h"
#include "ObjectManager.h"


BuiltinDirectoryPlugin::BuiltinDirectoryPlugin( QObject* parent ) :
	QObject( parent ),
	m_configuration(),
	m_commands( {
{ QStringLiteral("help"), tr( "Show help for specific command" ) },
{ QStringLiteral("add"), tr( "Add a location or computer" ) },
{ QStringLiteral("clear"), tr( "Clear all locations and computers" ) },
{ QStringLiteral("dump"), tr( "Dump all or individual locations and computers" ) },
{ QStringLiteral("list"), tr( "List all locations and computers" ) },
{ QStringLiteral("remove"), tr( "Remove a location or computer" ) },
{ QStringLiteral("import"), tr( "Import objects from given file" ) },
{ QStringLiteral("export"), tr( "Export objects to given file" ) },
				} )
{
}



void BuiltinDirectoryPlugin::upgrade( const QVersionNumber& oldVersion )
{
	if( oldVersion < QVersionNumber( 1, 1 ) &&
		m_configuration.localDataNetworkObjects().isEmpty() == false )
	{
		m_configuration.setNetworkObjects( m_configuration.localDataNetworkObjects() );
		m_configuration.setLocalDataNetworkObjects( QJsonArray() );
	}
}



NetworkObjectDirectory *BuiltinDirectoryPlugin::createNetworkObjectDirectory( QObject* parent )
{
	return new BuiltinDirectory( m_configuration, parent );
}



ConfigurationPage *BuiltinDirectoryPlugin::createConfigurationPage()
{
	return new BuiltinDirectoryConfigurationPage( m_configuration );
}



QStringList BuiltinDirectoryPlugin::commands() const
{
	return m_commands.keys();
}



QString BuiltinDirectoryPlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_help( const QStringList& arguments )
{
	const auto command = arguments.value( 0 );

	if( command == QStringLiteral("import") )
	{
		CommandLineIO::print( tr("\nUSAGE\n\n%1 import <FILE> [location <LOCATION>] [format <FORMAT-STRING-WITH-VARIABLES>] "
								 "[regex <REGULAR-EXPRESSION-WITH-VARIABLES>]\n\n"
								 "Valid variables: %type% %name% %host% %mac% %location%\n\n"
								 "Examples:\n\n"
								 "* Import simple CSV file to a single room:\n\n"
								 "    %1 import computers.csv location \"Room 01\" format \"%name%;%host%;%mac%\"\n\n"
								 "* Import CSV file with location name in first column:\n\n"
								 "    %1 import computers-with-rooms.csv format \"%location%,%name%,%mac%\"\n\n"
								 "* Import text file with with key/value pairs using regular expressions:\n\n"
								 "    %1 import hostlist.txt location \"Room 01\" regex \"^NAME:(%name%:.*)\\s+HOST:(%host%:.*)$\"\n\n"
								 "* Import arbitrarily formatted data:\n\n"
								 "    %1 import data.txt regex '^\"(%location%:[^\"]+)\";\"(%host%:[a-z\\d\\.]+)\".*$'\n").
							  arg( commandLineModuleName() ) );

		return NoResult;
	}
	else if( command == QStringLiteral("export") )
	{
		CommandLineIO::print( tr("\nUSAGE\n\n%1 export <FILE> [location <LOCATION>] [format <FORMAT-STRING-WITH-VARIABLES>]\n\n"
								 "Valid variables: %type% %name% %host% %mac% %location%\n\n"
								 "Examples:\n\n"
								 "* Export all objects to a CSV file:\n\n"
								 "    %1 export objects.csv format \"%type%;%name%;%host%;%mac%\"\n\n"
								 "* Export all computers in a location to a CSV file:\n\n"
								 "    %1 export computers.csv location \"Room 01\" format \"%name%;%host%;%mac%\"\n\n").
							  arg( commandLineModuleName() ) );

		return NoResult;
	}
	else if( command == QStringLiteral("add") )
	{
		CommandLineIO::print( tr("\nUSAGE\n\n%1 add <TYPE> <NAME> [<HOST ADDRESS> <MAC ADDRESS> <PARENT>]\n\n"
								 "Adds an object where TYPE can be one of \"%2\" or \"%3\". PARENT can be specified by name or UUID.\n\n"
								 "Examples:\n\n"
								 "* Add a room:\n\n"
								 "    %1 add location \"Room 01\"\n\n"
								 "* Add a computer to room \"Room 01\":\n\n"
								 "    %1 add computer \"Computer 01\" comp01.example.com 11:22:33:44:55:66 \"Room 01\"\n\n").
							  arg( commandLineModuleName(), QStringLiteral("location"), QStringLiteral("computer") ) );

		return NoResult;
	}
	else if( command == QStringLiteral("remove") )
	{
		CommandLineIO::print( tr("\nUSAGE\n\n%1 remove <OBJECT>\n\n"
								 "Removes the specified object from the directory. OBJECT can be specified by name or UUID. "
								 "Removing a location will also remove all related computers.\n\n"
								 "Examples:\n\n"
								 "* Remove a computer by name:\n\n"
								 "    %1 remove \"Computer 01\"\n\n"
								 "* Remove an object by UUID:\n\n"
								 "    %1 remove 068914fc-0f87-45df-a5b9-099a2a6d9141\n\n").
							  arg( commandLineModuleName(), QStringLiteral("location"), QStringLiteral("computer") ) );

		return NoResult;
	}

	return Unknown;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_add( const QStringList& arguments )
{
	if( arguments.count() < 2 )
	{
		return NotEnoughArguments;
	}

	NetworkObject object;

	const auto type = arguments[0];
	const auto name = arguments[1];

	if( type == QStringLiteral("location") )
	{
		object = NetworkObject( NetworkObject::Location, name );
	}
	else if( type == QStringLiteral("computer") )
	{
		auto hostAddress = arguments.value( 2 );
		if( hostAddress.isEmpty() )
		{
			hostAddress = name;
		}
		const auto macAddress = arguments.value( 3 );
		const auto parent = findNetworkObject( arguments.value( 4 ) );
		object = NetworkObject( NetworkObject::Host, name, hostAddress, macAddress,
								QString(), NetworkObject::Uid(), parent.isValid() ? parent.uid() : NetworkObject::Uid() );
	}
	else
	{
		CommandLineIO::error( tr("Invalid type specified. Valid values are \"%1\" or \"%2\"." ).
							  arg( QStringLiteral("computer"), QStringLiteral("location") ) );
		return Failed;
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
	objectManager.add( object );
	m_configuration.setNetworkObjects( objectManager.objects() );

	return saveConfiguration();
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_clear( const QStringList& arguments )
{
	Q_UNUSED(arguments);

	m_configuration.setNetworkObjects( {} );

	return saveConfiguration();
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_dump( const QStringList& arguments )
{
	CommandLineIO::TableHeader tableHeader( { tr("Object UUID"), tr("Parent UUID"), tr("Type"), tr("Name"), tr("Host address"), tr("MAC address") } );
	CommandLineIO::TableRows tableRows;

	const auto objects = m_configuration.networkObjects();

	tableRows.reserve( objects.size() );

	if( arguments.isEmpty() )
	{
		for( const auto& networkObjectValue : objects )
		{
			tableRows.append( dumpNetworkObject( networkObjectValue.toObject() ) );
		}
	}
	else
	{
		tableRows.append( dumpNetworkObject( findNetworkObject( arguments.first() ) ) );
	}

	CommandLineIO::printTable( CommandLineIO::Table( tableHeader, tableRows ) );

	return NoResult;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_list( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		listObjects( m_configuration.networkObjects(), NetworkObject::None );
	}
	else
	{
		const auto parents = BuiltinDirectory( m_configuration, this ).queryObjects( NetworkObject::Location, arguments.first() );

		for( const auto& parent : parents )
		{
			listObjects( m_configuration.networkObjects(), parent );
		}
	}

	return NoResult;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_remove( const QStringList& arguments )
{
	if( arguments.isEmpty() )
	{
		return NotEnoughArguments;
	}

	const auto object = findNetworkObject( arguments.first() );

	if( object.isValid() )
	{
		ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );
		objectManager.remove( object, true );
		m_configuration.setNetworkObjects( objectManager.objects() );

		return saveConfiguration();
	}

	CommandLineIO::error( tr( "Specified object not found." ) );

	return Failed;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_import( const QStringList& arguments )
{
	if( arguments.count() < 3 )
	{
		return NotEnoughArguments;
	}

	const auto& inputFileName = arguments.first();
	QFile inputFile( inputFileName );

	if( inputFile.exists() == false )
	{
		CommandLineIO::error( tr( "File \"%1\" does not exist!" ).arg( inputFileName ) );
		return Failed;
	}
	else if( inputFile.open( QFile::ReadOnly | QFile::Text ) == false )
	{
		CommandLineIO::error( tr( "Can't open file \"%1\" for reading!" ).arg( inputFileName ) );
		return Failed;
	}

	QString location;
	QString formatString;
	QString regularExpression;

	for( int i = 1; i < arguments.count(); i += 2 )
	{
		if( i+1 >= arguments.count() )
		{
			return NotEnoughArguments;
		}

		const auto key = arguments[i];
		const auto value = arguments[i+1];
		if( key == QStringLiteral("location") )
		{
			location = value;
		}
		else if( key == QStringLiteral("format") )
		{
			formatString = value;
		}
		else if( key == QStringLiteral("regex") )
		{
			regularExpression = value;
		}
		else
		{
			CommandLineIO::error( tr( "Unknown argument \"%1\"." ).arg( key ) );
			return InvalidArguments;
		}
	}

	if( formatString.isEmpty() == false )
	{
		regularExpression = formatString;

		const auto variables = fileImportVariables();

		for( const auto& var : variables )
		{
			regularExpression.replace( var, QStringLiteral("(%1:[^\\n\\r]*)").arg( var ) );
		}
	}

	if( regularExpression.isEmpty() == false )
	{
		if( importFile( inputFile, regularExpression, location ) )
		{
			return saveConfiguration();
		}

		return Failed;
	}

	CommandLineIO::error( tr("No format string or regular expression specified!") );

	return InvalidArguments;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::handle_export( const QStringList& arguments )
{
	if( arguments.count() < 3 )
	{
		return NotEnoughArguments;
	}

	const auto& outputFileName = arguments.first();
	QFile outputFile( outputFileName );

	if( outputFile.open( QFile::WriteOnly | QFile::Truncate | QFile::Text ) == false )
	{
		CommandLineIO::error( tr( "Can't open file \"%1\" for writing!" ).arg( outputFileName ) );
		return Failed;
	}

	QString location;
	QString formatString;

	for( int i = 1; i < arguments.count(); i += 2 )
	{
		if( i+1 >= arguments.count() )
		{
			return NotEnoughArguments;
		}

		const auto key = arguments[i];
		const auto value = arguments[i+1];
		if( key == QStringLiteral("location") )
		{
			location = value;
		}
		else if( key == QStringLiteral("format") )
		{
			formatString = value;
		}
		else
		{
			CommandLineIO::error( tr( "Unknown argument \"%1\"." ).arg( key ) );
			return InvalidArguments;
		}
	}

	if( formatString.isEmpty() == false )
	{
		if( exportFile( outputFile, formatString, location ) )
		{
			return Successful;
		}

		return Failed;
	}

	CommandLineIO::error( tr("No format string specified!") );

	return InvalidArguments;
}



void BuiltinDirectoryPlugin::listObjects( const QJsonArray& objects, const NetworkObject& parent )
{
	for( const auto& networkObjectValue : objects )
	{
		const NetworkObject networkObject( networkObjectValue.toObject() );

		if( ( parent.type() == NetworkObject::None && networkObject.parentUid().isNull() ) ||
			networkObject.parentUid() == parent.uid() )
		{
			printf( "%s\n", qUtf8Printable( listNetworkObject( networkObject ) ) );
			listObjects( objects, networkObject );
		}
	}
}



QStringList BuiltinDirectoryPlugin::dumpNetworkObject( const NetworkObject& object )
{
	return { VeyonCore::formattedUuid( object.uid() ),
				VeyonCore::formattedUuid( object.parentUid() ),
				networkObjectTypeName( object ),
				object.name(),
				object.hostAddress(),
				object.macAddress() };
}



QString BuiltinDirectoryPlugin::listNetworkObject( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::Location:
		return tr( "Location \"%1\"" ).arg( object.name() );
	case NetworkObject::Host:
		return QLatin1Char('\t') +
				tr( "Computer \"%1\" (host address: \"%2\" MAC address: \"%3\")" ).
				arg( object.name(), object.hostAddress(), object.macAddress() );
	default:
		break;
	}

	return tr( "Unclassified object \"%1\" with ID \"%2\"" ).arg( object.name(), object.uid().toString() );
}



QString BuiltinDirectoryPlugin::networkObjectTypeName( const NetworkObject& object )
{
	switch( object.type() )
	{
	case NetworkObject::None: return tr( "None" );
	case NetworkObject::Location: return typeNameLocation();
	case NetworkObject::Host: return typeNameComputer();
	case NetworkObject::Root: return typeNameRoot();
	default:
		break;
	}

	return tr( "Invalid" );
}



NetworkObject::Type BuiltinDirectoryPlugin::parseNetworkObjectType( const QString& typeName )
{
	if( typeName == typeNameLocation() )
	{
		return NetworkObject::Location;
	}
	else if( typeName == typeNameComputer() )
	{
		return NetworkObject::Host;
	}
	else if( typeName == typeNameRoot() )
	{
		return NetworkObject::Root;
	}

	return NetworkObject::None;
}



CommandLinePluginInterface::RunResult BuiltinDirectoryPlugin::saveConfiguration()
{
	ConfigurationManager configurationManager;
	if( configurationManager.saveConfiguration() == false )
	{
		CommandLineIO::error( configurationManager.errorString() );
		return Failed;
	}

	return Successful;
}



bool BuiltinDirectoryPlugin::importFile( QFile& inputFile,
										 const QString& regExWithVariables,
										 const QString& location )
{
	int lineCount = 0;
	QMap<QString, NetworkObjectList > networkObjects;
	while( inputFile.atEnd() == false )
	{
		++lineCount;

		auto targetLocation = location;

		const auto line = inputFile.readLine();
		const auto networkObject = toNetworkObject( QString::fromUtf8( line ), regExWithVariables, targetLocation );

		if( networkObject.isValid() )
		{
			networkObjects[targetLocation].append( networkObject );
		}
		else
		{
			CommandLineIO::error( tr( "Error while parsing line %1." ).arg( lineCount ) );
			return false;
		}
	}

	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );

	for( auto it = networkObjects.constBegin(), end = networkObjects.constEnd(); it != end; ++it )
	{
		auto parentLocation = objectManager.findByName( it.key() );
		auto parentLocationUid = parentLocation.uid();
		if( it.key().isEmpty() )
		{
			parentLocationUid = QUuid();
		}
		else if( parentLocation.isValid() == false )
		{
			parentLocation = NetworkObject( NetworkObject::Location, it.key() );
			objectManager.add( parentLocation );
			parentLocationUid = parentLocation.uid();
		}

		for( const NetworkObject& networkObject : qAsConst(it.value()) )
		{
			objectManager.add( NetworkObject( networkObject.type(),
											  networkObject.name(),
											  networkObject.hostAddress(),
											  networkObject.macAddress(),
											  QString(), NetworkObject::Uid(),
											  parentLocationUid ) );
		}
	}

	m_configuration.setNetworkObjects( objectManager.objects() );

	return true;
}



bool BuiltinDirectoryPlugin::exportFile( QFile& outputFile, const QString& formatString, const QString& location )
{
	ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );

	const auto& networkObjects = objectManager.objects();

	NetworkObject locationObject;
	if( location.isEmpty() == false )
	{
		locationObject = objectManager.findByName( location );
	}

	QStringList lines;
	lines.reserve( networkObjects.count() );

	for( auto it = networkObjects.constBegin(), end = networkObjects.constEnd(); it != end; ++it )
	{
		const NetworkObject networkObject( it->toObject() );

		auto currentLocation = location;

		if( locationObject.isValid() )
		{
			if( networkObject.parentUid() != locationObject.uid() )
			{
				continue;
			}
		}
		else
		{
			currentLocation = objectManager.findByUid( networkObject.parentUid() ).name();
		}

		lines.append( toFormattedString( networkObject, formatString, currentLocation ) );
	}

	// append empty string to generate final newline at end of file
	lines += QString();

	outputFile.write( lines.join( QStringLiteral("\r\n") ).toUtf8() );

	return true;
}



NetworkObject BuiltinDirectoryPlugin::findNetworkObject( const QString& uidOrName ) const
{
	const ObjectManager<NetworkObject> objectManager( m_configuration.networkObjects() );

	if( QUuid( uidOrName ).isNull() )
	{
		return objectManager.findByName( uidOrName );
	}

	return objectManager.findByUid( uidOrName );
}



NetworkObject BuiltinDirectoryPlugin::toNetworkObject( const QString& line, const QString& regExWithVariables, QString& location )
{
	QStringList variables;
	QRegExp varDetectionRX( QStringLiteral("\\((%\\w+%):[^)]+\\)") );
	int pos = 0;

	while( ( pos = varDetectionRX.indexIn( regExWithVariables, pos ) ) != -1 )
	{
		variables.append( varDetectionRX.cap(1) );
		pos += varDetectionRX.matchedLength();
	}

	QString rxString = regExWithVariables;
	for( const auto& var : qAsConst(variables) )
	{
		rxString.replace( QStringLiteral("%1:").arg( var ), QString() );
	}

	QRegExp rx( rxString );
	if( rx.indexIn( line ) != -1 )
	{
		auto objectType = NetworkObject::Host;
		const auto typeIndex = variables.indexOf( QStringLiteral("%type%") );
		if( typeIndex != -1 )
		{
			objectType = parseNetworkObjectType( rx.cap( 1 + typeIndex ) );
		}

		const auto locationIndex = variables.indexOf( QStringLiteral("%location%") );
		const auto nameIndex = variables.indexOf( QStringLiteral("%name%") );
		const auto hostIndex = variables.indexOf( QStringLiteral("%host%") );
		const auto macIndex = variables.indexOf( QStringLiteral("%mac%") );

		auto name = ( nameIndex != -1 ) ? rx.cap( 1 + nameIndex ).trimmed() : QString();
		auto host = ( hostIndex != -1 ) ? rx.cap( 1 + hostIndex ).trimmed() : QString();
		auto mac = ( macIndex != -1 ) ? rx.cap( 1 + macIndex ).trimmed() : QString();

		if( objectType == NetworkObject::Location )
		{
			return NetworkObject( NetworkObject::Location, name );
		}

		if( location.isEmpty() && locationIndex != -1 )
		{
			location = rx.cap( 1 + locationIndex ).trimmed();
		}

		if( host.isEmpty() )
		{
			host = name;
		}
		else if( name.isEmpty() )
		{
			name = host;
		}
		return NetworkObject( NetworkObject::Host, name, host, mac );
	}

	return NetworkObject::None;
}



QString BuiltinDirectoryPlugin::toFormattedString( const NetworkObject& networkObject,
												   const QString& formatString,
												   const QString& location )
{
	return QString( formatString ).
			replace( QStringLiteral("%location%"), location ).
			replace( QStringLiteral("%name%"), networkObject.name() ).
			replace( QStringLiteral("%host%"), networkObject.hostAddress() ).
			replace( QStringLiteral("%mac%"), networkObject.macAddress() ).
			replace( QStringLiteral("%type%"), networkObjectTypeName( networkObject ) );
}



QStringList BuiltinDirectoryPlugin::fileImportVariables()
{
	return { QStringLiteral("%location%"), QStringLiteral("%name%"), QStringLiteral("%host%"), QStringLiteral("%mac%"), QStringLiteral("%type%") };
}



IMPLEMENT_CONFIG_PROXY(BuiltinDirectoryConfiguration, &VeyonCore::config())
