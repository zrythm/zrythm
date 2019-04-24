/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_SQLITE_HEADER__
#define __JUCETICE_SQLITE_HEADER__

#if JUCE_SUPPORT_SQLITE

class SqliteConnection;

//==============================================================================
class SqliteResultset
{
public:

    //==============================================================================
    enum ColumnType
    {
        Unknown   = 0,
        Null      = 1,
        Integer   = 2,
        Double    = 3,
        Text      = 4,
        Blob      = 5
    };

    //==============================================================================
    /** Constructor */
    SqliteResultset (SqliteConnection* connection,
                     void* statement,
                     const String& errorText);
    
    /** Destructor */
    ~SqliteResultset ();

    //==============================================================================
    /**
    */
    bool isValid () const;

    /**
    */
    const String getErrorText () const;

    /**
    */
    bool next ();

    //==============================================================================
    /**
    */
    int columnCount ();

    /**
    */    
    int getColumnType (const int columnIndex);
    
    /**
    */    
    const String getColumnName (const int columnIndex);

    //==============================================================================
    /**
    */    
    bool columnIsNull (const int columnIndex);

    /**
    */    
    int columnAsInteger (const int columnIndex);

    /**
    */    
    double columnAsDouble (const int columnIndex);

    /**
    */    
    const String columnAsText (const int columnIndex);

    /**
    */    
    const void* columnAsBlob (const int columnIndex, int& bytesRead);

private:

    SqliteConnection* connection;
    void* statement;
    String errorText;
};


//==============================================================================
class SqliteConnection
{
public:

    //==============================================================================
    /** Constructor */
    SqliteConnection ();

    /** Destructor */
    ~SqliteConnection ();

    //==============================================================================
    /** Opens the connection to the SQLite database.
    
        Opens the connection to the SQLite database. Clients must call this method
        before "executeQuery()" or they will get a runtime error.
    
    */
    const String openDatabaseFile (const File& file);

    //==============================================================================
    /** Executes the query passed as parameter.

        This is the heart of the class; this method takes a SQL query
        in a string and sets the internal state of the class to inform clients
        about the result of the query.
        
        Remember to not store the returning resultset, which may live only if this
        instance of 
        
        @param query             The query to execute.
        @return SqliteResultset  the resultset class instance even in case of failure
    */
    SqliteResultset* executeQuery (const String& sqlText);

    //==============================================================================
    /** Returns the column names of the table passed as parameter.
        
        Returns a map with the name and type of the columns of the table
        whose name is passed as parameter.
    
        @param tableName         The name of the table whose schema is sought.
        @return A StringPairArray with pairs representing: [column name = column type]
    */
    const StringPairArray getTableSchema (const String& tableName);

private:

    friend class SqliteResultset;

    /** Removes a resultset from our list, eventually freeing the object memory */
    void removeResultset (SqliteResultset* resultSet, const bool deleteObject = false);

    void* database;
  	CriticalSection queriesLock;
    OwnedArray<SqliteResultset> queries;
};

#endif // JUCE_SUPPORT_SQLITE

#endif // __JUCETICE_SQLITE_HEADER__

