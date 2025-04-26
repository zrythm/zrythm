<#
    rubberband-sharp

    A consumer of the rubberband DLL that wraps the RubberBandStretcher
    object exposed by the DLL in a managed .NET type.

    Copyright 2018-2019 Jonathan Gilbert

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the name of Jonathan Gilbert
    shall not be used in advertising or otherwise to promote the sale,
    use or other dealings in this Software without prior written
    authorization.
#>

Param
(
  $InstallPath,
  $ToolsPath,
  $Package,
  $Project
)

# Save current project state.
$Project.Save()

# Load project XML.
$ProjectFullName = $Project.FullName

$Doc = New-Object System.Xml.XmlDocument
$Doc.Load($ProjectFullName)

$Namespace = 'http://schemas.microsoft.com/developer/msbuild/2003'

# Find the node containing the file. The tag "Content" may be replace by "None" depending of the case, check your .csproj file.
$ContentNodes = Select-Xml "//msb:Project/msb:ItemGroup/msb:Content[starts-with(@Include, 'rubberband-dll-') and (substring(@Include, string-length(@Include) - 3) = '.dll')]" $Doc -Namespace @{msb = $Namespace}

#check if the node exists.
If ($ContentNodes -ne $Null)
{
	$CopyNodeName = "CopyToOutputDirectory"

	ForEach ($ContentNode In $ContentNodes)
	{
		$NoneNode = $Doc.CreateElement("None", $Namespace)
		$NoneNode.SetAttribute("Include", $ContentNode.Node.Attributes["Include"].Value)
		
		$CopyNode = $Doc.CreateElement($CopyNodeName, $Namespace)
		$CopyNode.InnerText = "PreserveNewest"
		
		$NoneNode.AppendChild($CopyNode)

		$ContentNode.Node.ParentNode.ReplaceChild($NoneNode, $ContentNode.Node)
	}

	$DTE = $Project.DTE

	$Project.Select("vsUISelectionTypeSelect")

	$DTE.ExecuteCommand("Project.UnloadProject")
	$Doc.Save($ProjectFullName)
	$DTE.ExecuteCommand("Project.ReloadProject")
}
