// Import necessary libraries for the program
using Google.FlatBuffers;          // Library for working with FlatBuffers binary format
using reflection;                  // Library for FlatBuffers schema reflection (reading .bfbs files)
using System;                      // Core .NET system classes
using System.Collections.Generic;  // Collection classes like List, Dictionary, HashSet
using System.IO;                   // File input/output operations
using System.Linq;                 // LINQ query operations
using System.Text;                 // String manipulation classes like StringBuilder

// Main class that handles the conversion from FlatBuffers to Fast DDS IDL format
public class FlatBuffersToFastDdsConverter
{
    // Private fields to store important data throughout the object's lifetime
    private readonly string bfbsFilePath;                          // Path to the input .bfbs (binary FlatBuffers schema) file
    private readonly string baseDirectory;                         // Directory where the .bfbs file is located
    private reflection.Schema schema;                              // The loaded FlatBuffers schema object
    private readonly HashSet<string> processedTypes;              // Set to track which types we've already processed (prevents duplicates)
    private readonly Dictionary<string, string> includeToIdlMap;  // Maps include file names to their corresponding IDL file names
    private readonly List<string> processedIncludes;              // List of include files we've already processed

    // Constructor - called when creating a new instance of this class
    public FlatBuffersToFastDdsConverter(string bfbsFilePath)
    {
        // Check if the file path parameter is null, empty, or just whitespace
        if (string.IsNullOrWhiteSpace(bfbsFilePath))
            throw new ArgumentException(".bfbs file is null or empty", nameof(bfbsFilePath)); // Throw error with descriptive message

        // Check if the file actually exists on the file system
        if (!File.Exists(bfbsFilePath))
            throw new FileNotFoundException($".bfbs file not found: {bfbsFilePath}"); // Throw error if file doesn't exist

        // Initialize class fields with provided data and default values
        this.bfbsFilePath = Path.GetFullPath(bfbsFilePath);        // Convert to absolute path to avoid path issues
        this.baseDirectory = Path.GetDirectoryName(this.bfbsFilePath) ?? ""; // Get directory containing the .bfbs file
        this.processedTypes = new HashSet<string>();               // Initialize empty set for tracking processed types
        this.includeToIdlMap = new Dictionary<string, string>();   // Initialize empty dictionary for include mappings
        this.processedIncludes = new List<string>();               // Initialize empty list for processed includes

        // Try to load the schema, but handle different types of exceptions differently
        try
        {
            LoadSchema(); // Call method to load the FlatBuffers schema
        }
        catch (Exception ex) when (!(ex is ArgumentException || ex is FileNotFoundException)) // Catch all exceptions except the ones we already handle
        {
            // Wrap the exception in a more descriptive error message
            throw new InvalidOperationException($"Failed to load FlatBuffers schema from {this.bfbsFilePath}: {ex.Message}", ex);
        }
    }

    // Private method to load the FlatBuffers schema from the .bfbs file
    private void LoadSchema()
    {
        try
        {
            byte[] schemaBytes = File.ReadAllBytes(bfbsFilePath);           // Read the entire .bfbs file into a byte array
            ByteBuffer schemaBuf = new ByteBuffer(schemaBytes);            // Create a FlatBuffers ByteBuffer from the byte array
            schema = reflection.Schema.GetRootAsSchema(schemaBuf);         // Parse the ByteBuffer into a Schema object

            // Validate that the schema loaded correctly by trying to access a property
            int test = schema.EnumsLength; // This will throw an exception if the schema is invalid
        }
        catch (Exception ex)
        {
            // If anything goes wrong, wrap it in a more descriptive error
            throw new InvalidOperationException($"Failed to load FlatBuffers schema from {bfbsFilePath}: {ex.Message}", ex);
        }
    }

    // Public method that converts the schema to an IDL string and returns it
    public string ConvertToString()
    {
        var sb = new StringBuilder();                                      // Create a StringBuilder to efficiently build the output string
        var converter = new IdlConverter(schema, sb, includeToIdlMap);    // Create a converter object to do the actual conversion work
        return converter.Convert();                                       // Call the convert method and return the resulting IDL string
    }

    // Public method that converts the schema to IDL files on disk, returns list of created file paths
    public List<string> ConvertToFiles(string outputDirectory = "IDL", string fileName = null)
    {
        // If no output directory specified, use "IDL" as default
        if (string.IsNullOrEmpty(outputDirectory))
            outputDirectory = "IDL";

        Directory.CreateDirectory(outputDirectory);        // Create the output directory if it doesn't exist
        var generatedFiles = new List<string>();           // List to track all files we create

        ProcessIncludes(outputDirectory, generatedFiles);  // Process any included schema files first

        // If no filename specified, generate one from the input file
        if (string.IsNullOrEmpty(fileName))
        {
            string baseFileName = Path.GetFileNameWithoutExtension(bfbsFilePath); // Get filename without extension
            fileName = $"{baseFileName}";                                         // Use that as our output filename
        }

        string outputPath = Path.Combine(outputDirectory, $"{fileName}.idl");    // Create full path for output file
        string idlContent = ConvertToString();                                   // Convert schema to IDL string

        File.WriteAllText(outputPath, idlContent);                              // Write the IDL content to the file
        generatedFiles.Add(Path.GetFullPath(outputPath));                      // Add the file path to our list of generated files

        return generatedFiles; // Return list of all files we created
    }

    // Public method that converts to files but only returns the main output file path
    public string ConvertToFile(string outputDirectory = "IDL", string fileName = null)
    {
        var files = ConvertToFiles(outputDirectory, fileName); // Call the method that returns all files
        return files.LastOrDefault();                          // Return the last file (main output file)
    }

    // Private method to process any included schema files
    private void ProcessIncludes(string outputDirectory, List<string> generatedFiles)
    {
        // Check if we have a valid base directory to work with
        if (string.IsNullOrWhiteSpace(baseDirectory) || !Directory.Exists(baseDirectory))
        {
            Console.WriteLine($"Base directory '{baseDirectory}' is invalid or doesn't exist. Skipping include processing.");
            return; // Exit early if we can't process includes
        }

        // Look for the corresponding .fbs file (FlatBuffers source file)
        var mainFbsFile = Path.ChangeExtension(bfbsFilePath, ".fbs"); // Change .bfbs extension to .fbs
        if (!File.Exists(mainFbsFile))
        {
            Console.WriteLine($".fbs file not found: {mainFbsFile}");
            return; // Exit early if source file doesn't exist
        }

        // Extract list of included files from the .fbs source file
        var includedFiles = ExtractIncludesFromFbsFile(mainFbsFile);
        Console.WriteLine($"Found {includedFiles.Count} includes"); // Log how many includes we found

        // Process each included file
        foreach (var includedFile in includedFiles)
        {
            ProcessIncludedFile(includedFile, outputDirectory, generatedFiles);
        }
    }

    // Private method to parse a .fbs file and extract the list of included files
    private List<string> ExtractIncludesFromFbsFile(string fbsFilePath)
    {
        var includes = new List<string>(); // List to store found include paths

        try
        {
            string content = File.ReadAllText(fbsFilePath);                                    // Read the entire .fbs file as text
            var lines = content.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries); // Split into individual lines

            // Look through each line for include statements
            foreach (var line in lines)
            {
                var trimmedLine = line.Trim(); // Remove leading/trailing whitespace

                // Check if this line is an include statement
                if (trimmedLine.StartsWith("include "))
                {
                    var includePath = ExtractIncludePath(trimmedLine); // Extract the file path from the include line
                    if (!string.IsNullOrWhiteSpace(includePath))
                    {
                        // Convert relative paths to absolute paths
                        string absoluteIncludePath = Path.IsPathRooted(includePath)
                            ? includePath                                    // Already absolute
                            : Path.Combine(baseDirectory, includePath);    // Make relative path absolute

                        // Check if the included file actually exists
                        if (File.Exists(absoluteIncludePath))
                        {
                            includes.Add(absoluteIncludePath);                                    // Add to our list
                            Console.WriteLine($"  Found include: {includePath} -> {absoluteIncludePath}");
                        }
                        else
                        {
                            Console.WriteLine($"  Include file not found: {includePath} -> {absoluteIncludePath}");
                        }
                    }
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Could not parse .fbs file {fbsFilePath}: {ex.Message}");
        }

        return includes; // Return the list of found include files
    }

    // Private method to extract the file path from an include line
    private string ExtractIncludePath(string includeLine)
    {
        try
        {
            var pathPart = includeLine.Substring(8).Trim();           // Remove "include " (8 characters) from the beginning
            pathPart = pathPart.Trim(' ', '"', ';', '\t');           // Remove quotes, semicolons, and whitespace
            return pathPart;                                          // Return the cleaned path
        }
        catch
        {
            return null; // Return null if we can't extract the path
        }
    }

    // Private method to process a single included file
    private void ProcessIncludedFile(string fbsFilePath, string outputDirectory, List<string> generatedFiles)
    {
        try
        {
            // Validate input parameter
            if (string.IsNullOrWhiteSpace(fbsFilePath))
            {
                Console.WriteLine("Warning: Empty FBS file path provided");
                return;
            }

            string bfbsPath = Path.ChangeExtension(fbsFilePath, ".bfbs"); // Convert .fbs path to .bfbs path

            // Check if the corresponding .bfbs file exists
            if (string.IsNullOrWhiteSpace(bfbsPath) || !File.Exists(bfbsPath))
            {
                Console.WriteLine($".bfbs file not found for include: {bfbsPath ?? "null"}");
                return;
            }

            string fullBfbsPath = Path.GetFullPath(bfbsPath);             // Get absolute path
            if (processedIncludes.Contains(fullBfbsPath))                 // Check if we've already processed this file
            {
                return; // Skip if already processed
            }

            processedIncludes.Add(fullBfbsPath);                          // Mark as processed

            Console.WriteLine($"Processing included schema: {Path.GetFileName(fbsFilePath)}");

            // Create a new converter for this included file
            var includeConverter = new FlatBuffersToFastDdsConverter(bfbsPath);
            string includeFileName = Path.GetFileNameWithoutExtension(fbsFilePath); // Get filename without extension

            // Validate that we got a valid filename
            if (string.IsNullOrWhiteSpace(includeFileName))
            {
                Console.WriteLine($"Could not extract filename from {fbsFilePath}");
                return;
            }

            string includeIdlFileName = $"{includeFileName}.idl";                     // Create IDL filename
            string includeOutputPath = Path.Combine(outputDirectory, includeIdlFileName); // Create full output path
            string includeIdlContent = includeConverter.ConvertToString();           // Convert the include to IDL

            File.WriteAllText(includeOutputPath, includeIdlContent);                // Write IDL content to file
            generatedFiles.Add(Path.GetFullPath(includeOutputPath));               // Add to list of generated files

            includeToIdlMap[includeFileName] = includeIdlFileName;                  // Map include name to IDL filename
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Error processing include {fbsFilePath ?? "null"}: {ex.Message}");
        }
    }

    // Public method to get information about the loaded schema
    public SchemaInfo GetSchemaInfo()
    {
        int includeCount = GetIncludeCountFromSchema();           // Get count of processed includes
        return new SchemaInfo                                     // Create and return schema info object
        {
            FilePath = bfbsFilePath,                             // Path to the .bfbs file
            EnumCount = schema.EnumsLength,                      // Number of enums in the schema
            ObjectCount = schema.ObjectsLength,                  // Number of objects/structs in the schema
            PackageName = ExtractPackageName(),                  // Extract package/namespace name
            IncludeCount = includeCount                          // Number of included files
        };
    }

    // Private method to get the count of processed includes
    private int GetIncludeCountFromSchema()
    {
        return processedIncludes.Count; // Return count of items in the processed includes list
    }

    // Private method to extract the package/namespace name from the schema
    private string ExtractPackageName()
    {
        if (schema.ObjectsLength == 0) return "Generated";       // Default name if no objects

        var firstObject = schema.Objects(0).Value;               // Get the first object in the schema
        string fullName = firstObject.Name;                      // Get its full name (includes namespace)

        int lastDotIndex = fullName.LastIndexOf('.');            // Find the last dot in the name
        return lastDotIndex > 0 ? fullName.Substring(0, lastDotIndex) : "Generated"; // Extract namespace part
    }

    // Private nested class that handles the actual IDL conversion logic
    private class IdlConverter
    {
        // Private fields for the converter
        private readonly reflection.Schema schema;                        // The FlatBuffers schema to convert
        private readonly StringBuilder sb;                                // StringBuilder to build the output
        private readonly Dictionary<string, string> includeToIdlMap;     // Map of include files to IDL files

        // Constructor for the IDL converter
        public IdlConverter(reflection.Schema schema, StringBuilder sb, Dictionary<string, string> includeToIdlMap)
        {
            this.schema = schema;                                                              // Store the schema
            this.sb = sb;                                                                      // Store the StringBuilder
            this.includeToIdlMap = includeToIdlMap ?? new Dictionary<string, string>();      // Store include map (create empty if null)
        }

        // Main conversion method that orchestrates the entire conversion process
        public string Convert()
        {
            AddHeader();           // Add file header comments
            AddIncludes();         // Add include statements
            AddModule();           // Add the main module with all types
            return sb.ToString();  // Return the completed IDL string
        }

        // Private method to add header comments to the IDL file
        private void AddHeader()
        {
            sb.AppendLine("// Generated IDL file for Fast DDS");      // Add comment about file purpose
            sb.AppendLine("// Generated from FlatBuffers schema");    // Add comment about source
            sb.AppendLine();                                           // Add blank line for readability
        }

        // Private method to add include statements for any dependency files
        private void AddIncludes()
        {
            if (includeToIdlMap.Count > 0)                            // Check if we have any includes
            {
                foreach (var idlFile in includeToIdlMap.Values)      // Loop through each IDL file
                {
                    sb.AppendLine($"#include \"{idlFile}\"");        // Add include statement
                }
                sb.AppendLine();                                      // Add blank line after includes
            }
        }

        // Private method to add the main module structure
        private void AddModule()
        {
            string fullModuleName = ExtractModuleName();             // Get the module name from the schema
            string[] moduleParts = fullModuleName.Split('.');        // Split nested modules (e.g., "MyGame.Sample")

            // Create nested modules for namespaces like "MyGame.Sample"
            foreach (string part in moduleParts)
            {
                sb.AppendLine($"module {part} {{");                 // Open each module level
            }
            sb.AppendLine();                                         // Add blank line

            AddEnums();                                              // Add all enum definitions
            AddStructs();                                            // Add all struct definitions

            // Close nested modules in reverse order
            for (int i = 0; i < moduleParts.Length; i++)
            {
                sb.AppendLine("};");                                 // Close each module level
            }
        }

        // Private method to extract the module name from the schema
        private string ExtractModuleName()
        {
            if (schema.ObjectsLength == 0) return "Generated";      // Default if no objects

            var firstObject = schema.Objects(0).Value;              // Get first object
            string fullName = firstObject.Name;                     // Get its full name

            int lastDotIndex = fullName.LastIndexOf('.');           // Find namespace separator
            return lastDotIndex > 0 ? fullName.Substring(0, lastDotIndex) : "Generated"; // Extract namespace
        }

        // Private method to add all enum definitions to the IDL
        private void AddEnums()
        {
            for (int i = 0; i < schema.EnumsLength; i++)            // Loop through all enums in schema
            {
                var enumObj = schema.Enums(i).Value;                // Get enum object
                AddEnum(enumObj);                                   // Add this enum to the IDL
            }
        }

        // Private method to add a single enum definition
        private void AddEnum(reflection.Enum enumObj)
        {
            sb.AppendLine($"    enum {GetSimpleName(enumObj.Name)} {{");  // Start enum definition

            // Add each enum value
            for (int j = 0; j < enumObj.ValuesLength; j++)
            {
                var enumVal = enumObj.Values(j).Value;                     // Get enum value
                string comma = (j < enumObj.ValuesLength - 1) ? "," : "";  // Add comma except for last item
                sb.AppendLine($"        {enumVal.Name}{comma}");           // Add enum value line
            }

            sb.AppendLine("    };");                                       // Close enum definition
            sb.AppendLine();                                               // Add blank line
        }

        // Private method to add all struct definitions to the IDL
        private void AddStructs()
        {
            for (int i = 0; i < schema.ObjectsLength; i++)          // Loop through all objects in schema
            {
                var obj = schema.Objects(i).Value;                  // Get object
                AddStruct(obj);                                     // Add this struct to the IDL
            }
        }

        // Private method to add a single struct definition
        private void AddStruct(reflection.Object obj)
        {
            sb.AppendLine($"    struct {GetSimpleName(obj.Name)} {{");    // Start struct definition

            // Process fields, separating regular fields from unions
            var processedUnionTypes = new HashSet<string>();              // Track union types we've processed

            // Loop through all fields in the struct
            for (int j = 0; j < obj.FieldsLength; j++)
            {
                var field = obj.Fields(j).Value;                         // Get field object
                var type = field.Type.Value;                             // Get field type

                if (type.BaseType == BaseType.Union)                     // Handle union fields
                {
                    AddUnionField(field);
                }
                else if (type.BaseType == BaseType.UType)                // Skip union discriminator fields
                {
                    // Skip UType fields - these are union discriminators that we handle in AddUnionField
                    // We don't want separate fields for these
                    continue;
                }
                else                                                      // Handle regular fields
                {
                    AddRegularField(field);
                }
            }

            sb.AppendLine("    };");                                      // Close struct definition
            sb.AppendLine();                                              // Add blank line
        }

        // Private method to add a regular (non-union) field to the struct
        private void AddRegularField(reflection.Field field)
        {
            var type = field.Type.Value;                                 // Get field type
            string fieldName = field.Name;                               // Get field name

            // FIX NEEDED: This method needs to detect enum fields properly
            // Currently "color:Color" field becomes "char color" instead of "Color color"
            // Need to check if field references an enum by examining field metadata or
            // implementing proper enum field detection logic
            string idlType = GetFieldTypeString(type);                   // Convert type to IDL type string
            sb.AppendLine($"        {idlType} {fieldName};");            // Add field line
        }

        // Private method to convert a FlatBuffers type to IDL type string
        private string GetFieldTypeString(reflection.Type type)
        {
            switch (type.BaseType)                                       // Check the base type
            {
                case BaseType.Bool: return "boolean";                   // Boolean type
                case BaseType.Byte:
                    // FIX NEEDED: This always returns "char" but should detect if this is an enum field
                    // For "color:Color" field, this should return "Color" not "char"
                    // Need to add logic to check if this byte field is actually an enum reference
                    return "char";                                       // 8-bit signed integer
                case BaseType.UByte: return "octet";                    // 8-bit unsigned integer
                case BaseType.Short: return "short";                    // 16-bit signed integer
                case BaseType.UShort: return "unsigned short";          // 16-bit unsigned integer
                case BaseType.Int: return "long";                       // 32-bit signed integer
                case BaseType.UInt: return "unsigned long";             // 32-bit unsigned integer
                case BaseType.Long: return "long long";                 // 64-bit signed integer
                case BaseType.ULong: return "unsigned long long";       // 64-bit unsigned integer
                case BaseType.Float: return "float";                    // 32-bit floating point
                case BaseType.Double: return "double";                  // 64-bit floating point
                case BaseType.String: return "string";                  // String type

                case BaseType.Vector:                                    // Array/vector type
                    // Get the element type for vectors
                    string elementType = GetElementTypeString(type);     // Get type of elements in the vector
                    return $"sequence<{elementType}>";                  // Return IDL sequence type

                case BaseType.Obj:                                      // Reference to another object
                    // This handles references to other structs/tables
                    if (type.Index < schema.ObjectsLength)              // Check bounds
                    {
                        var obj = schema.Objects((int)type.Index).Value; // Get referenced object
                        return GetSimpleName(obj.Name);                  // Return simple name
                    }
                    return "octet";                                      // Default fallback

                case BaseType.UType:                                     // Union type discriminator
                    // This is typically the enum type for unions
                    if (type.Index < schema.EnumsLength)                // Check bounds
                    {
                        var enumObj = schema.Enums((int)type.Index).Value; // Get enum object
                        return GetSimpleName(enumObj.Name);              // Return simple name
                    }
                    return "octet";                                      // Default fallback

                default:
                    return "octet";                                      // Default for unknown types
            }
        }

        // Private method to get the element type for vectors/arrays
        private string GetElementTypeString(reflection.Type vectorType)
        {
            switch (vectorType.Element)                                  // Check the element type
            {
                case BaseType.Bool: return "boolean";                   // Boolean elements
                case BaseType.Byte: return "char";                      // Byte elements
                case BaseType.UByte: return "octet";                    // Unsigned byte elements
                case BaseType.Short: return "short";                    // Short elements
                case BaseType.UShort: return "unsigned short";          // Unsigned short elements
                case BaseType.Int: return "long";                       // Integer elements
                case BaseType.UInt: return "unsigned long";             // Unsigned integer elements
                case BaseType.Long: return "long long";                 // Long elements
                case BaseType.ULong: return "unsigned long long";       // Unsigned long elements
                case BaseType.Float: return "float";                    // Float elements
                case BaseType.Double: return "double";                  // Double elements
                case BaseType.String: return "string";                  // String elements

                case BaseType.Obj:                                      // Object elements
                    // FIX NEEDED: Add bounds checking here
                    // Should check if vectorType.Index < schema.ObjectsLength before accessing
                    var obj = schema.Objects((int)vectorType.Index).Value; // Get object type
                    return GetSimpleName(obj.Name);                      // Return simple name

                default:
                    return "octet";                                      // Default for unknown types
            }
        }

        // Private method to add a union field to the struct
        private void AddUnionField(reflection.Field field)
        {
            var type = field.Type.Value;                                 // Get field type
            string unionName = field.Name;                               // Get field name

            // FIX NEEDED: Add bounds checking here  
            // Should check if type.Index < schema.EnumsLength before accessing
            var unionEnum = schema.Enums((int)type.Index).Value;        // Get union enum

            sb.AppendLine($"        // Union field: {unionName}");       // Add comment
            sb.AppendLine($"        {GetSimpleName(unionEnum.Name)} {unionName}_type;"); // Add type discriminator field

            // Create a union for the different types
            sb.AppendLine($"        union {unionName}_data switch({GetSimpleName(unionEnum.Name)}) {{"); // Start union definition

            // Add each possible union type
            for (int u = 0; u < unionEnum.ValuesLength; u++)
            {
                var unionVal = unionEnum.Values(u).Value;               // Get union value
                if (unionVal.Name.Equals("NONE", StringComparison.OrdinalIgnoreCase)) // Skip NONE value
                    continue;

                // Find corresponding object type
                var unionObjName = FindUnionObjectName(unionVal.Name);  // Find matching object name
                if (!string.IsNullOrEmpty(unionObjName))
                {
                    // Add case for this union type
                    sb.AppendLine($"            case {unionVal.Name}: {GetSimpleName(unionObjName)} {unionVal.Name.ToLower()}_value;");
                }
            }

            sb.AppendLine($"        }} {unionName}_value;");             // Close union definition
        }

        // Private method to find the object name corresponding to a union value
        private string FindUnionObjectName(string unionValueName)
        {
            for (int i = 0; i < schema.ObjectsLength; i++)              // Loop through all objects
            {
                var obj = schema.Objects(i).Value;                      // Get object
                string simpleName = GetSimpleName(obj.Name);            // Get simple name
                if (simpleName.Equals(unionValueName, StringComparison.OrdinalIgnoreCase)) // Check if names match
                {
                    return obj.Name;                                     // Return full object name
                }
            }
            return null;                                                 // Return null if not found
        }

        // Private method to extract the simple name from a fully qualified name
        private string GetSimpleName(string fullName)
        {
            int lastDotIndex = fullName.LastIndexOf('.');               // Find last dot
            return lastDotIndex >= 0 ? fullName.Substring(lastDotIndex + 1) : fullName; // Return part after last dot
        }
    }
}

// Data class to hold information about a schema
public class SchemaInfo
{
    public string FilePath { get; set; }          // Path to the schema file
    public int EnumCount { get; set; }            // Number of enums in the schema
    public int ObjectCount { get; set; }          // Number of objects in the schema
    public string PackageName { get; set; }       // Package/namespace name
    public int IncludeCount { get; set; }         // Number of included files

    // Override ToString to provide a nice string representation of the schema info
    public override string ToString()
    {
        return $"Schema: {Path.GetFileName(FilePath)} | Package: {PackageName} | Enums: {EnumCount} | Objects: {ObjectCount} | Includes: {IncludeCount}";
    }
}

// Main program class - this is where execution starts
class Program
{
    // Main method - the entry point of the program
    static void Main(string[] args)
    {
        try
        {
            // Get command line arguments or use defaults
            string inputFile = args.Length > 0 ? args[0] : "monster.bfbs";  // First argument is input file, default to "monster.bfbs"
            string outputDir = args.Length > 1 ? args[1] : "IDL";           // Second argument is output directory, default to "IDL"

            // Create the converter object
            var converter = new FlatBuffersToFastDdsConverter(inputFile);   // This loads and validates the schema
            
            // Convert the schema to IDL files
            var outputPaths = converter.ConvertToFiles(outputDir);          // This does the actual conversion

            // Print success message and results
            Console.WriteLine($"Conversion completed successfully");
            Console.WriteLine($"Generated {outputPaths.Count} IDL file(s):");
            foreach (var path in outputPaths)                               // Print each generated file path
            {
                Console.WriteLine($"  - {path}");
            }

            // Get and display schema information
            var schemaInfo = converter.GetSchemaInfo();                     // Get schema statistics
            Console.WriteLine($"\nSchema Info: {schemaInfo}");              // Print schema info
        }
        catch (Exception ex)                                                // Catch any errors that occur
        {
            Console.WriteLine($"Error: {ex.Message}");                      // Print error message
        }
    }
}

/*
STEP-BY-STEP EXECUTION FLOW:

1. PROGRAM STARTUP:
   - Program starts at Main() method
   - Command line arguments are checked (args array)
   - If no arguments provided, defaults are used: "monster.bfbs" for input, "IDL" for output directory

2. CONVERTER CREATION:
   - new FlatBuffersToFastDdsConverter(inputFile) is called
   - Constructor validates the input file path exists
   - Sets up internal data structures (HashSet, Dictionary, List)
   - Calls LoadSchema() to read and parse the .bfbs file
   - LoadSchema() reads the binary file into a byte array
   - Creates a ByteBuffer and parses it into a Schema object
   - Validates the schema by accessing a property

3. CONVERSION PROCESS:
   - converter.ConvertToFiles(outputDir) is called
   - Creates the output directory if it doesn't exist
   - Calls ProcessIncludes() to handle any included schema files:
     * Looks for corresponding .fbs source file
     * Parses the .fbs file to find "include" statements
     * For each include found:
       - Converts the .fbs path to .bfbs path
       - Creates a new converter for the included file
       - Recursively converts the included schema to IDL
       - Saves the included IDL file
       - Maps the include name to IDL filename
   - Calls ConvertToString() to convert the main schema
   - ConvertToString() creates a StringBuilder and IdlConverter
   - IdlConverter.Convert() orchestrates the conversion:
     * AddHeader() - adds comment header
     * AddIncludes() - adds #include statements for dependencies
     * AddModule() - creates the main module structure

4. MODULE GENERATION:
   - ExtractModuleName() gets namespace from first object in schema
   - Creates nested module declarations (e.g., "module MyGame { module Sample {")
   - AddEnums() processes all enums in the schema:
     * For each enum, creates "enum EnumName { VALUE1, VALUE2, ... };"
   - AddStructs() processes all objects/tables in the schema:
     * For each object, creates "struct StructName { ... };"
     * AddRegularField() handles normal fields:
       - Converts FlatBuffers types to IDL types (bool->boolean, int->long, etc.)
       - Handles vectors as "sequence<ElementType>"
       - Handles object references by name
     * AddUnionField() handles union fields:
       - Creates a discriminator field for the union type
       - Creates a union declaration with cases for each possible type
   - Closes all module declarations

5. FILE OUTPUT:
   - Writes the generated IDL content to the output file
   - Adds the file path to the list of generated files
   - Returns the list of all generated files

6. RESULTS DISPLAY:
   - Main() receives the list of generated files
   - Prints success message and file paths
   - Calls GetSchemaInfo() to get statistics:
     * Counts enums and objects in the schema
     * Extracts package name from object names
     * Counts processed includes
   - Creates and returns a SchemaInfo object
   - Prints the schema statistics

7. ERROR HANDLING:
   - Any exceptions during the process are caught in Main()
   - Error messages are displayed to the console
   - Program exits gracefully

KEY DATA FLOW:
- Input: .bfbs (binary FlatBuffers schema) file
- Processing: Parses binary schema, extracts type information
- Output: .idl (Interface Definition Language) files for Fast DDS
- The conversion maps FlatBuffers concepts to DDS concepts:
  * Tables become structs
  * Enums remain enums  
  * Vectors become sequences
  * Unions become discriminated unions
  * Namespaces become modules

IMPORTANT NOTES:
- The code has several "FIX NEEDED" comments indicating known issues
- Enum field detection needs improvement (currently maps enum fields as "char" instead of the enum type)
- Bounds checking is missing in some array/vector access operations
- The conversion is one-way: FlatBuffers -> IDL (not bidirectional)
*/
