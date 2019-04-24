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

#if JUCE_SUPPORT_SQLITE

extern "C" {
  #include <sqlite3.h>
}

BEGIN_JUCE_NAMESPACE

//==============================================================================
SqliteResultset::SqliteResultset (SqliteConnection* connection_,
                                  void* statement_,
                                  const String& errorText_)
    : connection (connection_),
      statement ((sqlite3_stmt*) statement_),
      errorText (errorText_)
{
}

SqliteResultset::~SqliteResultset ()
{
    if (statement)
        sqlite3_finalize ((sqlite3_stmt*) statement);

    if (connection)
        connection->removeResultset (this, false);
}

bool SqliteResultset::isValid () const
{
    return statement != 0;
}

const String SqliteResultset::getErrorText () const
{
    return errorText;
}

bool SqliteResultset::next ()
{
    return (sqlite3_step ((sqlite3_stmt*) statement) == SQLITE_ROW);
}

int SqliteResultset::columnCount ()
{
    return sqlite3_column_count ((sqlite3_stmt*) statement);
}

int SqliteResultset::getColumnType (const int columnIndex)
{
    int columnType = sqlite3_column_type ((sqlite3_stmt*) statement, columnIndex);

    switch (columnType) {
        case SQLITE_NULL:    return SqliteResultset::Null;
        case SQLITE_INTEGER: return SqliteResultset::Integer;
        case SQLITE_FLOAT:   return SqliteResultset::Double;
        case SQLITE_TEXT:    return SqliteResultset::Text;
        case SQLITE_BLOB:    return SqliteResultset::Blob;
        default:             return SqliteResultset::Unknown;
    }
}

const String SqliteResultset::getColumnName (int columnIndex)
{
    return String (sqlite3_column_name ((sqlite3_stmt*) statement, columnIndex));
}

bool SqliteResultset::columnIsNull (const int columnIndex)
{
    return sqlite3_column_type ((sqlite3_stmt*) statement, columnIndex) == SQLITE_NULL;
}

int SqliteResultset::columnAsInteger (const int columnIndex)
{
    return sqlite3_column_int ((sqlite3_stmt*) statement, columnIndex);
}

double SqliteResultset::columnAsDouble (const int columnIndex)
{
    return sqlite3_column_double ((sqlite3_stmt*) statement, columnIndex);
}

const String SqliteResultset::columnAsText (const int columnIndex)
{
    return String ((const char*) sqlite3_column_text ((sqlite3_stmt*) statement, columnIndex));
}

const void* SqliteResultset::columnAsBlob (const int columnIndex, int& bytesRead)
{
    bytesRead = sqlite3_column_bytes ((sqlite3_stmt*) statement, columnIndex);

    return sqlite3_column_blob ((sqlite3_stmt*) statement, columnIndex);
}


//==============================================================================
SqliteConnection::SqliteConnection ()
    : database (0)
{
}

SqliteConnection::~SqliteConnection ()
{
    if (database)
        sqlite3_close ((sqlite3*) database);
}

const String SqliteConnection::openDatabaseFile (const File& file)
{
    String errorText;

    if (database)
        sqlite3_close ((sqlite3*) database);

    if (file.existsAsFile ())
    {
        sqlite3* db = 0;
        if (sqlite3_open ((const char*) file.getFullPathName(), &db))
        {
            errorText = "Error connecting to " + file.getFullPathName();
        }
        else
        {
            database = db;
        }
    }
    else
    {
        errorText = "Database file " + file.getFullPathName() + " does not exists";
    }
    
    return errorText;
}

SqliteResultset* SqliteConnection::executeQuery (const String& sqlText)
{
    sqlite3_stmt* statement = 0;
    String errorText;

    if (database)
    {
        int rc = sqlite3_prepare_v2 ((sqlite3*) database, (const char*) sqlText, -1, &statement, 0);
        if (rc != SQLITE_OK)
        {
            errorText = (const char*) sqlite3_errmsg ((sqlite3*) database);
        }
    }

    ScopedLock sl (queriesLock);

    SqliteResultset* queryResultSet = new SqliteResultset (this, statement, errorText);
    queries.add (queryResultSet);

    return queryResultSet;
}

const StringPairArray SqliteConnection::getTableSchema (const String& tableName)
{
    jassertfalse; // not implemented !    
    
    StringPairArray schema;

#if 0
    char* error;
    char** resultSet;
    int numRows, numColumns;

    String query;    
    query << "PRAGMA table_info(\"";
    query << tableName;
    query << "\");";
    
    int resultCode = sqlite3_get_table(
                        (sqlite3*) database,  // An open database
                        (const char*) query,  // SQL to be executed
                        &resultSet,           // Result written to a char*[] that this points to
                        &numRows,             // Number of result rows written here
                        &numColumns,          // Number of result columns written here
                        &error                // Error msg written here
    );
    
    if (resultCode == SQLITE_OK)
    {
        for (int row = numColumns; row < numRows; row += 6)
        {
            schema[String (resultSet[row + 1])] = String (resultSet[row + 2]);
        }
    }
    
    sqlite3_free_table (resultSet);
#endif

    return schema;
}

void SqliteConnection::removeResultset (SqliteResultset* resultSet, const bool deleteObject)
{
    ScopedLock sl (queriesLock);

    if (queries.contains (resultSet))
        queries.removeObject (resultSet, deleteObject);
}

END_JUCE_NAMESPACE

#endif // JUCE_SUPPORT_SQLITE

