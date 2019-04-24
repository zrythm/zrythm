// ---
//
// $Id: htmloutput.cpp,v 1.7 2008/07/15 20:33:31 hartwork Exp $
//
// CppTest - A C++ Unit Testing Framework
// Copyright (c) 2003 Niklas Lundell
//
// ---
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// ---

#include "cpptest-htmloutput.h"
#include "utils.h"

namespace Test
{
	namespace
	{
		void
		strreplace(std::string &value, char search, const std::string &replace)
		{
			std::string::size_type idx = 0;
			while((idx = value.find(search, idx)) != std::string::npos)
			{
				value.replace(idx, 1, replace);
				idx += replace.size();
			}
		}
		
		std::string
		escape(std::string value)
		{
			strreplace(value, '&', "&amp;");
			strreplace(value, '<', "&lt;");
			strreplace(value, '>', "&gt;");
			strreplace(value, '"', "&quot;");
			strreplace(value, '\'', "&#39;");
			return value;
		}
		
		void
		header(std::ostream& os, std::string name)
		{
			if (!name.empty())
				name += " ";
			name = escape(name);
			os <<
				"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
				"<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
				"<head>\n"
				"  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n"
				"  <meta name=\"generator\" content=\"CppTest - http://cpptest.sourceforge.net\" />\n"
				"  \n"
				"  <title>" << name << "Unit Tests Results</title>\n"
				"  \n"
				"  <style type=\"text/css\" media=\"screen\">\n"
				"    <!--\n"
				"    hr  {\n"
				"      width: 100%;\n"
				"      border-width: 0px;\n"
				"      height: 1px;\n"
				"      color: #cccccc;\n"
				"      background-color: #cccccc;\n"
				"      padding: 0px;\n"
				"    }\n"
				"    \n"
				"    table {\n"
				"      width:100%;\n"
				"      border-collapse:separate;\n"
				"      border-spacing: 2px;\n"
				"      border:0px;\n"
				"    }\n"
				"    tr {\n"
				"      margin:0px;\n"
				"      padding:0px;\n"
				"    }\n"
				"    td {\n"
				"      margin:0px;\n"
				"      padding:1px;\n"
				"    }\n"
				"    .table_summary {\n"
				"    }\n"
				"    .table_suites {\n"
				"    }\n"
				"    .table_suite {\n"
				"    }\n"
				"    .table_result {\n"
				"      margin: 0px 0px 1em 0px;\n"
				"    }\n"
				"    .tablecell_title {\n"
				"      background-color: #a5cef7;\n"
				"      font-weight: bold;\n"
				"    }\n"
				"    \n"
				"    .tablecell_success {\n"
				"      background-color: #efefe7;\n"
				"    }\n"
				"    \n"
				"    .tablecell_error {\n"
				"      color: #ff0808;\n"
				"      background-color: #efefe7;\n"
				"      font-weight: bold;\n"
				"    }\n"
				"    p.spaced {\n"
				"      margin: 0px;\n"
				"      padding: 1em 0px 2em 0px;\n"
				"    }\n"
				"    p.unspaced {\n"
				"      margin: 0px;\n"
				"      padding: 0px 0px 2em 0px;\n"
				"    }\n"
				"    -->\n"
				"  </style>\n"
				"</head>\n"
				"\n"
				"<body>\n"
				"\n"
				"<h1><a name=\"top\"></a>" << name << "Unit Tests Results</h1>\n"
				"\n"
				"<div style=\"text-align:right\">\n"
				"Designed by <a href=\"http://cpptest.sourceforge.net\">CppTest</a>\n"
				"</div>\n"
				"<hr />\n"
				"\n";
		}
		
		void
		footer(std::ostream& os)
		{
			os <<
				"\n"
				"<p>\n"
				"  <a href=\"http://validator.w3.org/#validate-by-upload\">\n"
				"    Valid XHTML 1.0 Strict\n"
				"  </a>\n"
				"</p>\n"
				"</body>\n</html>\n";
		}
				
		void
		back_ref(std::ostream& os, const std::string& ref, bool prepend_newline = true)
		{
			
			os	<< "<p class=\"" << (prepend_newline ? "spaced" : "unspaced") << "\"><a href=\"#" << ref
				<< "\">Back to " << escape(ref) << "</a>\n</p>\n";
		}
		
		void
		sub_title(std::ostream& os, const std::string& title, int size)
		{
			std::ostringstream h;
			h << "h" << size;
			os << "<" << h.str() << ">" << escape(title) << "</" << h.str() << ">\n";
		}

		void
		sub_title(std::ostream& os, const std::string& title, int size, const std::string& mark)
		{
			std::ostringstream h;
			h << "h" << size;
			os << "<" << h.str() << "><a name=\"" << mark << "\"></a>" << escape(title) << "</" << h.str() << ">\n";
		}
			
		enum ClassTableType { TableClass_Summary, TableClass_Suites, TableClass_Suite, TableClass_Result };
		
		void
		table_header(std::ostream& os, ClassTableType type, const std::string &summary = "")
		{
			static const char* class_tabletypes[] = { "summary", "suites", "suite", "result" };
			
			os << "<table summary=\"" << escape(summary) << "\" class=\"table_" << class_tabletypes[type] << "\">\n";
		}
		
		void
		table_footer(std::ostream& os)
		{
			os << "</table>\n";
		}
		
		void
		table_tr_header(std::ostream& os)
		{
			os << "  <tr>\n";
		}
		
		void
		table_tr_footer(std::ostream& os)
		{
			os << "  </tr>\n";
		}
		
		enum ClassType { Title, Success, Error };
		
		void
		table_entry(std::ostream& os, ClassType type, const std::string& s,
					int width = 0, const std::string& link = "")
		{
			static const char* class_types[] = { "title", "success", "error" };
			
			os << "    <td";
			if (width)
				os << " style=\"width:" << width << "%\"";
			if (!link.empty())
				os << " class=\"tablecell_" << class_types[type] << "\"><a href=\"#" << link << "\">" << escape(s) << "</a>";
			else
				os << " class=\"tablecell_" << class_types[type] << "\">" << escape(s);
			os << "</td>\n";	
		}
						
	} // anonymous namespace
	
	// Test suite table
	//
	struct HtmlOutput::SuiteRow
	{
		std::ostream& _os;
		SuiteRow(std::ostream& os) : _os(os) {}
		void operator()(const SuiteInfo& si)
		{
			ClassType 		type(si._errors > 0 ? Error : Success);
            std::ostringstream 	ss;

			table_tr_header(_os);
			  table_entry(_os, type, si._name, 0, si._name);
			  ss.str(""), ss << si._tests.size();
			  table_entry(_os, type, ss.str(), 10);
			  ss.str(""), ss << si._errors;
			  table_entry(_os, type, ss.str(), 10);
			  ss.str(""),  ss << correct(si._tests.size(), si._errors) << "%";
			  table_entry(_os, type, ss.str(), 10);
			  ss.str(""), ss << si._time;
			  table_entry(_os, type, ss.str(), 10);
			table_tr_footer(_os);
		}
	};
	
	// Individual tests tables, tests
	//
	struct HtmlOutput::TestRow
	{
		bool		_incl_ok_tests;
		std::ostream& 	_os;
		TestRow(std::ostream& os, bool incl_ok_tests)
			: _incl_ok_tests(incl_ok_tests), _os(os) {}
		void operator()(const TestInfo& ti)
		{
			if (!ti._success || _incl_ok_tests)
			{
				std::string			link = ti._success ? std::string(""):
									       ti._sources.front().suite() + "_" + ti._name;
				ClassType 		type(ti._success ? Success : Error);
				std::ostringstream 	ss;
				
				table_tr_header(_os);
				  table_entry(_os, type, ti._name, 0, link);
				  ss.str(""),  ss << ti._sources.size();
				  table_entry(_os, type, ss.str());
				  table_entry(_os, type, ti._success ? "true" : "false");
				  ss.str(""), ss << ti._time;
				  table_entry(_os, type, ss.str());
				table_tr_footer(_os);
			}
		}
	};
	
	// Individual tests tables, header
	//
	struct HtmlOutput::TestSuiteRow
	{
		bool		_incl_ok_tests;
		std::ostream& 	_os;
		TestSuiteRow(std::ostream& os, bool incl_ok_tests) 
			: _incl_ok_tests(incl_ok_tests), _os(os) {}
		void operator()(const SuiteInfo& si)
		{
			std::ostringstream ss;
			
			sub_title(_os, "Suite: " + si._name, 3, si._name);
			table_header(_os, TableClass_Suite, "Details for suite " + si._name);
			  table_tr_header(_os);
			    table_entry(_os, Title, "Name");
			    table_entry(_os, Title, "Errors", 10);
			    table_entry(_os, Title, "Success", 10);
			    table_entry(_os, Title, "Time (s)", 10);
			  table_tr_footer(_os);
			  std::for_each(si._tests.begin(), si._tests.end(), 
					        TestRow(_os, _incl_ok_tests));
			table_footer(_os);
			back_ref(_os, "top");
		}
	};
	
	// Individual tests result tables
	//
	struct HtmlOutput::TestResult
	{
		std::ostream& _os;
		TestResult(std::ostream& os) : _os(os) {}
		void operator()(const Source& s)
		{
			const int TitleSize = 15;
			
			std::ostringstream ss;
			
			table_header(_os, TableClass_Result, "Test Failure");
			  table_tr_header(_os);
				table_entry(_os, Title, "Test", TitleSize);
				table_entry(_os, Success, s.suite() + "::" + s.test());
			  table_tr_footer(_os);
			  table_tr_header(_os);
				table_entry(_os, Title, "File", TitleSize);
				ss << s.file() << ":" << s.line();
				table_entry(_os, Success, ss.str());
			  table_tr_footer(_os);
			  table_tr_header(_os);
				table_entry(_os, Title, "Message", TitleSize);
				table_entry(_os, Success, s.message());
			  table_tr_footer(_os);
			table_footer(_os);
		}
	};
	
	// All tests result tables
	//
	struct HtmlOutput::TestResultAll
	{
		std::ostream& _os;
		TestResultAll(std::ostream& os) : _os(os) {}
		void operator()(const TestInfo& ti)
		{
			if (!ti._success)
			{
				const std::string& suite = ti._sources.front().suite();
				
				sub_title(_os, suite + "::" + ti._name, 3, suite + "_" + ti._name);
				std::for_each(ti._sources.begin(), ti._sources.end(), TestResult(_os));
				back_ref(_os, suite, false);
			}
		}
	};
	
	// Individual tests result tables, iterator
	//
	struct HtmlOutput::SuiteTestResult
	{
		std::ostream& _os;
		SuiteTestResult(std::ostream& os) : _os(os) {}
		void operator()(const SuiteInfo& si)
		{
			std::for_each(si._tests.begin(), si._tests.end(), TestResultAll(_os));
		}
	};
	
	/// Generates the HTML table. This function should only be called after
	/// run(), when all tests have been executed.
	///
	/// \param os            Output stream.
	/// \param incl_ok_tests Set if successful tests should be shown; 
	///                      false otherwise.
	/// \param name          Name of generated report.
	///
    void
	HtmlOutput::generate(std::ostream& os, bool incl_ok_tests, const std::string& name)
	{
		ClassType 		type(_total_errors > 0 ? Error : Success);
		std::ostringstream 	ss;
		
		header(os, name);
		
		// Table: Summary
		//
		sub_title(os, "Summary", 2);
		table_header(os, TableClass_Summary, "Summary of test results");
		  table_tr_header(os);
		    table_entry(os, Title, "Tests", 30);
		    table_entry(os, Title, "Errors", 30);
		    table_entry(os, Title, "Success", 30);
		    table_entry(os, Title, "Time (s)", 10);
		  table_tr_footer(os);
		  table_tr_header(os);
		    ss.str(""), ss << _total_tests;
 		    table_entry(os, type, ss.str(), 30);
			ss.str(""),  ss << _total_errors;
		    table_entry(os, type, ss.str(), 30);
			ss.str(""),  ss << correct(_total_tests, _total_errors) << "%";
		    table_entry(os, type, ss.str(), 30);
			ss.str(""), ss << _total_time;
		    table_entry(os, type, ss.str(), 10);
		  table_tr_footer(os);
		table_footer(os);
		os << "<hr />\n\n";
		
		// Table: Test suites
		//
		sub_title(os, "Test suites", 2);
		table_header(os, TableClass_Suites, "Test Suites");
		  table_tr_header(os);
		    table_entry(os, Title, "Name");
		    table_entry(os, Title, "Tests",   10);
		    table_entry(os, Title, "Errors",  10);
		    table_entry(os, Title, "Success",    10);
		    table_entry(os, Title, "Time (s)", 10);
		  table_tr_footer(os);
		  std::for_each(_suites.begin(), _suites.end(), SuiteRow(os));
		table_footer(os);
		os << "<hr />\n\n";
		
		// Individual tests tables
		//
		std::for_each(_suites.begin(), _suites.end(), TestSuiteRow(os, incl_ok_tests));
		os << "<hr />\n\n";
		
		// Individual tests result tables
		//
		if(_total_errors != 0)
		{
			sub_title(os, "Test results", 2);
			std::for_each(_suites.begin(), _suites.end(), SuiteTestResult(os));
			os << "<hr />\n\n";
		}		
		// EOF
		//
		footer(os);
	}
	
} // namespace Test

