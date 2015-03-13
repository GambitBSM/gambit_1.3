#!/bin/python
#
# GAMBIT: Global and Modular BSM Inference Tool
#*********************************************
# \file
#
#  Script to find scanner (and test function)
#  libraries and write cmake_variables.hpp.in
#  file.
#
#*********************************************
#
#  Authors (add name and date if you modify):
#
#  \author Pat Scott 
#          (p.scott@imperial.ac.uk)
#  \date 2014 Dec
#  \date 2015 Jan
#  \date 2015 February -- J. Cornell
#    (removed -rdynamic from OSX linker flags)
#
#
#*********************************************
import re
import os
import update_cmakelists
import yaml
import shutil
import itertools
import datetime
import sys
import getopt

scan_config = "./config/scanner_locations.yaml"
test_config = "./config/objective_locations.yaml"

# Remove C/C++ comments from 'text' (From http://stackoverflow.com/questions/241327/python-snippet-to-remove-c-and-c-comments)
def comment_remover(text):
    def replacer(match):
        s = match.group(0)
        if s.startswith('/'):
            return ""
        else:
            return s
    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    return re.sub(pattern, replacer, text[:])

# No empties from re.split
def neatsplit(regex,string):
    return [x for x in re.split(regex,string) if x != '']

# Actual updater program
def main(argv):
    
    exclude_plugins=set([])
    plugins = []
    static_links = ""
    flag = {"missing": "0", "found": "1", "excluded": "2"}

    # Handle command line options
    verbose = False
    try:
        opts, args = getopt.getopt(argv,"vx:",["verbose","exclude-scanners="])
    except getopt.GetoptError:
        print 'Usage: locate_scanners.py [flags]'
        print ' flags:'
        print '        -v                       : More verbose output'  
        print '        -x scanner1,scanner2,... : Exclude scanner1, scanner2, etc.' 
        sys.exit(2)
    for opt, arg in opts:
        if opt in ('-v','--verbose'):
            verbose = True
            print 'locate_scanners.py: verbose=True'
        elif opt in ('-x','--exclude-plugins','--exclude-plugin'):
            exclude_plugins.update(neatsplit(",",arg))

    # info for the different plugin types
    src_paths = sorted(["./ScannerBit/src/scanners", "./ScannerBit/src/objectives"])
    inc_paths = sorted(["./ScannerBit/include/gambit/ScannerBit/scanners", "./ScannerBit/include/gambit/ScannerBit/objectives"])
    plug_type = sorted(["scanner", "objective"])
    
    # these map the linking flags and library paths to the appropriate plugin library
    scanbit_plugins = dict()
    scanbit_incs = dict()
    scanbit_libs = dict()
    scanbit_links = dict()
    scanbit_reqs = dict()
    scanbit_auto_libs = dict()
    scanbit_auto_incs = dict()
    scanbit_link_libs = dict()
    scanbit_lib_hints = dict()
    scanbit_inc_files = dict()
    #scanbit_static_links = dict()
    
    ## begin adding scannerbit files to CMakeLists.txt ##
    scanbit_srcs = [ name for name in os.listdir("./ScannerBit/src") if os.path.isfile('./ScannerBit/src/' + name) if name.endswith(".cpp") or name.endswith(".c") or name.endswith(".cc") or name.endswith(".cxx") ]
    scanbit_hdrs = [ name for name in os.listdir("./ScannerBit/include/gambit/ScannerBit") if os.path.isfile('./ScannerBit/include/gambit/ScannerBit/' + name) if name.endswith(".hpp") or name.endswith(".h") ]
    prior_srcs = []
    prior_hdrs = []
    if os.path.exists("./ScannerBit/src/priors"):
                 prior_srcs = [ root + "/" + f for root,dirs,files in os.walk("./ScannerBit/src/priors") for f in files if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx") ]
    if os.path.exists("./ScannerBit/include/gambit/ScannerBit/priors"):
                 prior_hdrs = [ root + "/" + f for root,dirs,files in os.walk("./ScannerBit/include/gambit/ScannerBit/priors") for f in files if f.endswith(".hpp") or f.endswith(".h") ]
    
    cmakelist_txt_out = "set( scannerbit_sources\n"
    prior_txt_out = "#ifndef PRIOR_LIST_HPP\n#define PRIOR_LIST_HPP\n\n"
            
    for source in sorted(scanbit_srcs):
        cmakelist_txt_out += " "*16 + "src/" + source + "\n"
        
    for source in sorted(prior_srcs):
        cmakelist_txt_out += " "*16 + "src/" + source.split('/ScannerBit/src/')[1] + "\n"
        
    cmakelist_txt_out += ")\n\n"
    
    cmakelist_txt_out += "set( scannerbit_headers\n"
            
    for header in sorted(scanbit_hdrs):
        cmakelist_txt_out += " "*16 + "include/gambit/ScannerBit/" + header + "\n"
        
    for header in sorted(prior_hdrs):
        cmakelist_txt_out += " "*16 + "include/gambit/ScannerBit/" + header.split('/ScannerBit/include/gambit/ScannerBit/')[1] + "\n"
        prior_txt_out += "#include \"" + "gambit/ScannerBit/" + header.split('/ScannerBit/include/gambit/ScannerBit/')[1] + "\"\n"
        
    cmakelist_txt_out += ")\n\n"
    prior_txt_out += "\n#endif\n"
    ## end adding scannerbit files to CMakeLists.txt ##
    
    ## begin adding scannbit executable files to CMakeLists.txt ##
    
    scan_helper_srcs = []
    scan_helper_hdrs = []
    if os.path.exists("./ScannerBit/src/scanlibs"):
                 scan_helper_srcs = [ root + "/" + f for root,dirs,files in os.walk("./ScannerBit/src/scanlibs") for f in files if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx") ]
    if os.path.exists("./ScannerBit/include/gambit/ScannerBit/scanlibs"):
                 scan_helper_hdrs = [ root + "/" + f for root,dirs,files in os.walk("./ScannerBit/include/gambit/ScannerBit/scanlibs") for f in files if f.endswith(".hpp") or f.endswith(".h") ]
    scan_standalone_srcs = []
    scan_standalone_hdrs = []
    if os.path.exists("./ScannerBit/examples"):
                 scan_standalone_srcs = [ root + "/" + f for root,dirs,files in os.walk("./ScannerBit/examples") for f in files if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx") ]
    if os.path.exists("./ScannerBit/examples"):
                 scan_standalone_hdrs = [ root + "/" + f for root,dirs,files in os.walk("./ScannerBit/examples") for f in files if f.endswith(".hpp") or f.endswith(".h") ]
    
    cmakelist_txt_out += "set( scanner_scanlibs_sources\n"
            
    for source in sorted(scan_helper_srcs):
        cmakelist_txt_out += " "*16 + "src/" + source.split('/ScannerBit/src/')[1] + "\n"
        
    cmakelist_txt_out += ")\n\n"
    
    cmakelist_txt_out += "set( scanner_scanlibs_headers\n"
    
    for header in sorted(scan_helper_hdrs):
        cmakelist_txt_out += " "*16 + "include/gambit/ScannerBit/" + header.split('/ScannerBit/include/gambit/ScannerBit/')[1] + "\n"
       
    cmakelist_txt_out += ")\n\n"
    
    cmakelist_txt_out += "set( scanbit_standalone_sources\n"
            
    for source in sorted(scan_standalone_srcs):
        cmakelist_txt_out += " "*16 + "examples/" + source.split('/ScannerBit/examples/')[1] + "\n"
        
    cmakelist_txt_out += ")\n\n"
    
    cmakelist_txt_out += "set( scanbit_standalone_headers\n"
    
    for header in sorted(scan_standalone_hdrs):
        cmakelist_txt_out += " "*16 + "examples/" + header.split('/ScannerBit/examples/')[1] + "\n"
       
    cmakelist_txt_out += ")\n\n"
    
    ## end adding scannbit executable files to CMakeLists.txt ##
    
    # loop through the different plugin types
    for i in xrange(len(plug_type)):
        scanbit_plugins[plug_type[i]] = dict()
        scanbit_incs[plug_type[i]] = dict()
        scanbit_libs[plug_type[i]] = dict()
        scanbit_links[plug_type[i]] = dict()
        scanbit_reqs[plug_type[i]] = dict()
        scanbit_auto_libs[plug_type[i]] = dict()
        scanbit_auto_incs[plug_type[i]] = dict()
        scanbit_link_libs[plug_type[i]] = dict()
        scanbit_lib_hints[plug_type[i]] = dict()
        scanbit_inc_files[plug_type[i]] = dict()
        #scanbit_static_links[plug_type[i]] = dict()
    
    # loop through the different plugin types
    for i in xrange(len(plug_type)):
        scanbit_plugins[plug_type[i]] = dict()
        #scanbit_static_links[plug_type[i]] = dict()
        directories = [ name for name in os.listdir(src_paths[i]) if os.path.isdir(src_paths[i] + "/" + name) ]
        
        for directory in sorted(directories):
            scanbit_plugins[plug_type[i]][directory] = []
            # Find all source files in the ScannerBit scanner and test_function plugin directories
            sources = [ root + "/" + f for root,dirs,files in os.walk(src_paths[i] + "/" + directory) for f in files if f.endswith(".cpp") or f.endswith(".c") or f.endswith(".cc") or f.endswith(".cxx") ]
            headers = []
            if os.path.exists(inc_paths[i] + "/" + directory):
                 headers = [ root + "/" + f for root,dirs,files in os.walk(inc_paths[i] + "/" + directory) for f in files if f.endswith(".hpp") or f.endswith(".h") ]
            
            cmakelist_txt_out = cmakelist_txt_out+"set( " + plug_type[i] + "_plugin_sources_" + directory + "\n"

            # Work through the source files to find all plugins that need external linkage
            for source in sorted(sources):
                with open(source) as f:
                    last_plugin = ""
                    last_version= ""
                    last_plugin_file=[]
                    processed = False
                    if verbose: print "  Scanning source file {0} for ScannerBit plugin declarations.".format(source)
                    text = comment_remover(f.read())
                    it = re.finditer(r'\breqd_inifile_entries\s*?\(.*?\)|\bREQD_INIFILE_ENTRIES\s*?\(.*?\)', text, re.DOTALL)
                    ini_finds = [[m.span()[0], -1, re.sub(r'\s', '', m.group())] for m in it]
                    it = re.finditer(r'\bobjective_plugin\s*?\(.*?\)\s*?\{', text, re.DOTALL)
                    obj_finds = [[m.span()[0], 0, m.group()] for m in it]
                    it = re.finditer(r'\bscanner_plugin\s*?\(.*?\)\s*?\{', text, re.DOTALL)
                    scan_finds = [[m.span()[0], 1, m.group()] for m in it]
                    it = re.finditer(r'\breqd_libraries\s*?\(.*?\)|\bREQD_LIBRARIES\s*?\(.*?\)', text, re.DOTALL)
                    lib_finds = [[m.span()[0], -2, re.sub(r'\s', '', m.group())] for m in it]
                    it = re.finditer(r'\breqd_headers\s*?\(.*?\)|\bREQD_HEADERS\s*?\(.*?\)', text, re.DOTALL)
                    inc_finds = [[m.span()[0], -3, re.sub(r'\s', '', m.group())] for m in it]
                    all_finds  = sorted(scan_finds + obj_finds + ini_finds + lib_finds + inc_finds)
                    for find in all_finds:
                        if find[1] >= 0:
                            processed = False
                            splitline = neatsplit(r'\(|\)|,|\s|\{',find[2])
                            if len(splitline) != 0: 
                                plugin_name = splitline[1]
                                mod_version = ["0","0","0",""]
                                plugin_type = plug_type[find[1]];
                                if splitline[2] == "version": mod_version[0:len(splitline[3:])] = splitline[3:]
                                token = plugin_name+"__t__"+plugin_type+"__v__"+"_".join([x for x in mod_version])
                                status = "ok"
                                for x in exclude_plugins: 
                                    if (plugin_name+"_"+"_".join([y for y in mod_version])).startswith(x):
                                        status = "excluded"
                                last_plugin_file=[plugin_name, plugin_type, mod_version, status, token, [], directory, plug_type[i]]
                                
                                last_plugin = plugin_name
                                last_version = mod_version[0] + "." + mod_version[1] + "." + mod_version[2]
                                
                                if mod_version[3] != "":
                                    last_version += "-" + mod_version[3]
                                
                                scanbit_plugins[plug_type[i]][directory] += [last_plugin_file]
                        
                        elif find[1] == -1:
                            if not scanbit_reqs[plug_type[i]].has_key(last_plugin):
                                scanbit_reqs[plug_type[i]][last_plugin] = dict()
                            if scanbit_reqs[plug_type[i]][last_plugin].has_key(last_version):
                                if scanbit_reqs[plug_type[i]][last_plugin][last_version][0] == "":
                                    scanbit_reqs[plug_type[i]][last_plugin][last_version][0] = find[2][21:-1]
                                else:
                                    scanbit_reqs[plug_type[i]][last_plugin][last_version][0] += "," + find[2][21:-1]
                            else:
                                scanbit_reqs[plug_type[i]][last_plugin][last_version] = [find[2][21:-1], "", "", [], [], [], []]
                        
                        elif find[1] == -2:
                            if not scanbit_auto_libs[plug_type[i]].has_key(directory):
                                scanbit_auto_libs[plug_type[i]][directory] = neatsplit(',|\"', find[2][15:-1])
                            else:
                                scanbit_auto_libs[plug_type[i]][directory] += neatsplit(',|\"', find[2][15:-1])
                                
                            if not scanbit_reqs[plug_type[i]].has_key(last_plugin):
                                scanbit_reqs[plug_type[i]][last_plugin] = dict()
                            if scanbit_reqs[plug_type[i]][last_plugin].has_key(last_version):
                                if scanbit_reqs[plug_type[i]][last_plugin][last_version][1] == "":
                                    scanbit_reqs[plug_type[i]][last_plugin][last_version][1] = find[2][15:-1]
                                else:
                                    scanbit_reqs[plug_type[i]][last_plugin][last_version][1] += "," + find[2][15:-1]
                            else:
                                scanbit_reqs[plug_type[i]][last_plugin][last_version] = ["", find[2][15:-1], "", [], [], [], []]
                            
                            if (not processed) and (last_plugin_file[3] != "excluded"):
                                last_plugin_file[3] = "link_needed"
                                processed = True
                                plugins += [last_plugin_file]
                                
                        elif find[1] == -3:    
                            if not scanbit_auto_incs[plug_type[i]].has_key(directory):
                                scanbit_auto_incs[plug_type[i]][directory] = neatsplit(',|\"', find[2][13:-1])
                            else:
                                scanbit_auto_incs[plug_type[i]][directory] += neatsplit(',|\"', find[2][13:-1])
                                
                            if not scanbit_reqs[plug_type[i]].has_key(last_plugin):
                                scanbit_reqs[plug_type[i]][last_plugin] = dict()
                            if scanbit_reqs[plug_type[i]][last_plugin].has_key(last_version):
                                if scanbit_reqs[plug_type[i]][last_plugin][last_version][2] == "":
                                    scanbit_reqs[plug_type[i]][last_plugin][last_version][2] = find[2][13:-1]
                                else:
                                    scanbit_reqs[plug_type[i]][last_plugin][last_version][2] += "," + find[2][13:-1]
                            else:
                                scanbit_reqs[plug_type[i]][last_plugin][last_version] = ["", "", find[2][13:-1], [], [], [], []]
                                
                            if (not processed) and (last_plugin_file[3] != "excluded"):
                                processed = True
                                plugins += [last_plugin_file]
                        
            ## begin adding plugin files to CMakeLists.txt ##
                cmakelist_txt_out += " "*16 + "src/" + source.split('/ScannerBit/src/')[1] + "\n"
                
            cmakelist_txt_out += ")\n\n"
            
            cmakelist_txt_out += "set( " + plug_type[i] + "_plugin_headers_" + directory + "\n"
            
            for header in sorted(headers):
                cmakelist_txt_out += " "*16 + "include/gambit/ScannerBit/" + header.split('/ScannerBit/include/gambit/ScannerBit/')[1] + "\n"
                
            cmakelist_txt_out += ")\n\n"
            ## end adding plugin files to CMakeLists.txt ## 
                
    for config_file, plugin_type in itertools.izip([scan_config, test_config], ["scan", "like"]):
        # Create the locations yaml files from the example if needed
        if not os.path.isfile(config_file): shutil.copyfile(config_file+".example",config_file)
        # Load the locations yaml file, and work out which libs are present
        yaml_file = yaml.load(open(config_file), yaml.cyaml.CLoader)
        for plugin in plugins:
            plugin_name = plugin[0]
            inc_commands = []
            auto_libs = []
            auto_incs = []
            linkcommands = ""
            linklibs = []
            #staticlinkcommands = ""
            linkdirs = []
            linkhints = []
            inc_files = []
            if yaml_file: 
                if plugin_name in yaml_file and plugin[1] == plugin_type:
                    version_bits = plugin[2]
                    version = ".".join([x for x in version_bits[0:3] if x != ""])
                    if version_bits[3] != "": version = "-".join([version, version_bits[3]])
                    ini_version = ""
                    if version in yaml_file[plugin_name]:
                        ini_version = version
                    elif "any_version" in yaml_file[plugin_name]:
                        ini_version = "any_version"
                    if ini_version != "":
                        options_list = []
                        if type(yaml_file[plugin_name][ini_version]) is list:
                            options_list = yaml_file[plugin_name][ini_version]
                        elif type(yaml_file[plugin_name][ini_version]) is dict:
                            options_list = [yaml_file[plugin_name][ini_version]]
                        else:
                            options_list = yaml_file[plugin_name][ini_version]
                        for f in options_list:
                            for key, value in f.iteritems():
                                if key in ("lib", "libs", "library", "libraries", "library_path", "library_paths", "-lib", "-libs", "-library", "-libraries", "-library_path", "-library_paths"):
                                    libs = neatsplit(',|\s|;', value)
                                    for lib in libs:
                                        if os.path.isfile(lib):
                                            lib_full = os.path.abspath(lib)
                                            print "   Found library {0} needed for ScannerBit plugin {1} v{2}".format(lib,plugin_name,version)
                                            if lib_full.endswith(".a"):
                                                static_links += lib_full + " "
                                                [libdir, lib] = os.path.split(lib_full)
                                                lib = re.sub(r"^lib|\..*$","",lib)
                                                #staticlinkcommands += "-L" + libdir + " -l" + lib + " "
                                            else:
                                                [libdir, lib] = os.path.split(lib_full)
                                                lib = re.sub(r"^lib|\..*$","",lib)
                                                linkcommands += "-L" + libdir + " -l" + lib + " "
                                                linkdirs += [libdir]
                                                
                                            linklibs += [[lib, lib_full]]
                                            scanbit_reqs[plugin[7]][plugin_name][version][3] += [lib]
                                        elif os.path.isdir(lib):
                                            lib = os.path.abspath(lib)
                                            print "   Found library path {0} needed for ScannerBit plugin {1} v{2}".format(lib,plugin_name,version)
                                            linkhints += [lib]
                                        elif lib == "ROOT" or lib == "GSL":
                                            auto_libs += [lib]
                                            #scanbit_reqs[plugin[7]][plugin_name][version][3] += [lib]
                                            if scanbit_reqs[plugin[7]][plugin_name][version][1] == "":
                                                scanbit_reqs[plugin[7]][plugin_name][version][1] = "\"" + lib + "\""
                                            else:
                                                scanbit_reqs[plugin[7]][plugin_name][version][1] += ",\"" + lib + "\""
                                        else:
                                            [libdir, lib] = os.path.split(lib)
                                            lib = re.sub("^lib|\..*$","",lib)
                                            scanbit_reqs[plugin[7]][plugin_name][version][4] += [lib]
                                            
                                elif key in ("inc", "incs", "include", "includes", "include_path", "include_paths", "-inc", "-incs", "-include", "-includes", "-include_path", "-include_paths", "hdr", "hdrs", "header", "headers", "-hdr", "-hdrs", "-header", "-headers"):
                                    incs = neatsplit(',|\s|;', value)
                                    for inc in incs:
                                        if os.path.isdir(inc):
                                            inc = os.path.abspath(inc)
                                            print "   Found include path {0} needed for ScannerBit plugin {1} v{2}".format(inc,plugin_name,version)
                                            inc_commands += [inc]
                                            scanbit_reqs[plugin[7]][plugin_name][version][5] += [inc]
                                        elif os.path.isfile(inc):
                                            inc = os.path.abspath(inc)
                                            print "   Found include file {0} needed for ScannerBit plugin {1} v{2}".format(inc,plugin_name,version)
                                            [incdir, inc] = os.path.split(inc)
                                            inc_commands += [incdir]
                                            scanbit_reqs[plugin[7]][plugin_name][version][5] += [incdir]
                                            inc_files += [[inc, incdir]]
                                        elif inc == "ROOT" or inc == "GSL":
                                            auto_incs += [inc]
                                            #scanbit_reqs[plugin[7]][plugin_name][version][5] += [inc]
                                            if scanbit_reqs[plugin[7]][plugin_name][version][2] == "":
                                                scanbit_reqs[plugin[7]][plugin_name][version][2] = "\"" + inc + "\""
                                            else:
                                                scanbit_reqs[plugin[7]][plugin_name][version][2] += ",\"" + inc + "\""
                                        else:
                                            plugin[3] = "missing"
                                            scanbit_reqs[plugin[7]][plugin_name][version][6] += [inc]
                                else:
                                    print "   Unknown infile option \"{0}\" needed for ScannerBit plugin {1} v{2}".format(key,plugin_name,version)
                        
                        # add links commands to map (keys: {plug_type, directory}) to be linked to later
                        #if staticlinkcommands != "":
                        #    if scanbit_static_links[plugin[7]].has_key(plugin[6]):
                        #        scanbit_static_links[plugin[7]][plugin[6]] += staticlinkcommands
                        #    else:
                        #        scanbit_static_links[plugin[7]][plugin[6]] = staticlinkcommands
                        if inc_commands != []:
                            if scanbit_incs[plugin[7]].has_key(plugin[6]):
                                scanbit_incs[plugin[7]][plugin[6]] += inc_commands
                            else:
                                scanbit_incs[plugin[7]][plugin[6]] = inc_commands
                        if linkcommands != "":
                            if scanbit_links[plugin[7]].has_key(plugin[6]):
                                scanbit_links[plugin[7]][plugin[6]] += linkcommands
                            else:
                                scanbit_links[plugin[7]][plugin[6]] = linkcommands
                        if linkdirs != []:
                            if scanbit_libs[plugin[7]].has_key(plugin[6]):
                                scanbit_libs[plugin[7]][plugin[6]] += linkdirs
                            else:
                                scanbit_libs[plugin[7]][plugin[6]] = linkdirs
                        if auto_libs != []:
                            if scanbit_auto_libs[plugin[7]].has_key(plugin[6]):
                                scanbit_auto_libs[plugin[7]][plugin[6]] += auto_libs
                            else:
                                scanbit_auto_libs[plugin[7]][plugin[6]] = auto_libs
                        if auto_incs != []:
                            if scanbit_auto_incs[plugin[7]].has_key(plugin[6]):
                                scanbit_auto_incs[plugin[7]][plugin[6]] += auto_incs
                            else:
                                scanbit_auto_incs[plugin[7]][plugin[6]] = auto_incs
                        if linklibs != []:
                            if scanbit_link_libs[plugin[7]].has_key(plugin[6]):
                                scanbit_link_libs[plugin[7]][plugin[6]] += linklibs
                            else:
                                scanbit_link_libs[plugin[7]][plugin[6]] = linklibs
                        if linkhints != []:
                            if scanbit_lib_hints[plugin[7]].has_key(plugin[6]):
                                scanbit_lib_hints[plugin[7]][plugin[6]] += linkhints
                            else:
                                scanbit_lib_hints[plugin[7]][plugin[6]] = linkhints
                        if inc_files != []:
                            if scanbit_inc_files[plugin[7]].has_key(plugin[6]):
                                scanbit_inc_files[plugin[7]][plugin[6]] += inc_files
                            else:
                                scanbit_inc_files[plugin[7]][plugin[6]] = inc_files
      
    # Make a candidate cmake_variables.hpp.in file
    towrite = "\
// GAMBIT: Global and Modular BSM Inference Tool  \n\
//************************************************\n\
/// \\file                                        \n\
///                                               \n\
///  prior_rollcall.hpp file for ScannerBit.      \n\
///                                               \n\
///  This file has been automatically generated by\n\
///  locate_scanners.py.  Please do not modify.   \n\
///                                               \n\
///***********************************************\n\
///                                               \n\
///  Authors:                                     \n\
///                                               \n\
///  \\author The GAMBIT Collaboration            \n\
///  \\date "+datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y")+"\n\
///                                               \n\
///***********************************************\n\n"

    towrite += prior_txt_out

    header = "./ScannerBit/include/gambit/ScannerBit/priors_rollcall.hpp"
    with open(header+".candidate","w") as f: f.write(towrite)
    update_cmakelists.update_only_if_different(header, header+".candidate")

    if verbose: print "Finished writing ScannerBit/include/gambit/ScannerBit/priors_rollcall.hpp"

    # Make a candidate linkedout.cmake file
    towrite = "\
# GAMBIT: Global and Modular BSM Inference Tool  \n\
#************************************************\n\
# \\file                                         \n\
#                                                \n\
#  Cmake CMakeLists.txt file for ScannerBit.     \n\
#                                                \n\
#  This file has been automatically generated by \n\
#  locate_scanners.py.  Please do not modify.    \n\
#                                                \n\
#************************************************\n\
#                                                \n\
#  Authors:                                      \n\
#                                                \n\
#  \\author The GAMBIT Collaboration             \n\
#  \\date "+datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y")+"\n\
#                                                \n\
#************************************************\n\
                                                 \n\
set( PLUGIN_INCLUDE_DIRECTORIES                  \n\
                ${PROJECT_BINARY_DIR}            \n\
                ${PROJECT_BINARY_DIR}/cmake      \n\
                ${GAMBIT_INCDIRS}                \n\
                ${mkpath_INCLUDE_DIR}            \n\
                ${yaml_INCLUDE_DIR}              \n\
                ${Boost_INCLUDE_DIR}             \n\
                ${GSL_INCLUDE_DIRS}              \n\
                ${ROOT_INCLUDE_DIR}              \n\
)                                                \n\n\
if( ${PLUG_VERBOSE} )                            \n\
    message(\"*** begin PLUG_INCLUDE_DIRECTORIES ***\")\n\
    foreach(dir ${PLUGIN_INCLUDE_DIRECTORIES})   \n\
        message(STATUS \"dir='${dir}'\")         \n\
    endforeach()                                 \n\
    message(\"*** end PLUG_INCLUDE_DIRECTORIES ***\")\n\
endif()                                          \n\n"

    towrite += cmakelist_txt_out

    towrite +="\n\
add_gambit_library( ScannerBit OPTION OBJECT SOURCES ${scannerbit_sources} HEADERS ${scannerbit_headers} )\n\n\
add_gambit_executable( scanlibs SOURCES ${scanner_scanlibs_sources} HEADERS ${scanner_scanlibs_headers})\n\
add_dependencies(scanlibs yaml-cpp)             \n\
set_target_properties( scanlibs                 \n\
                       PROPERTIES               \n\
                       RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/bin\")\n\n\
include(utils.cmake)                            \n\n\
set( scanbit_standalone_src                     \n\
        ${scanbit_standalone_sources}           \n\
        $<TARGET_OBJECTS:Logs>                  \n\
        $<TARGET_OBJECTS:Printers>              \n\
        $<TARGET_OBJECTS:ScannerBit>            \n\
        $<TARGET_OBJECTS:mkpath>                \n\
        ${utils_source_files}                   \n\
)                                               \n\n\
set( scanbit_standalone_hdr                     \n\
        ${scanbit_standalone_headers}           \n\
        ${utils_header_files}                   \n\
)                                               \n\n\
add_gambit_executable( scannerbit_standalone SOURCES ${scanbit_standalone_src} HEADERS ${scanbit_standalone_hdr})\n\
set_target_properties( scannerbit_standalone    \n\
                       PROPERTIES               \n\
                       RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/bin\")\n\
add_dependencies(scannerbit_standalone ScannerBit)\n\
add_dependencies(scannerbit_standalone mkpath)  \n\
add_dependencies(scannerbit_standalone yaml-cpp)\n\
target_link_libraries(scannerbit_standalone yaml-cpp)\n\
if (NOT EXCLUDE_FLEXIBLESUSY)                   \n\
  add_dependencies(scannerbit_standalone flexiblesusy)\n\
  target_link_libraries(scannerbit_standalone ${flexiblesusy_LDFLAGS})\n\
endif()                                         \n\
if (NOT EXCLUDE_DELPHES)                        \n\
  add_dependencies(scannerbit_standalone delphes)\n\
  target_link_libraries(scannerbit_standalone ${delphes_LDFLAGS} ${ROOT_LIBRARIES} ${ROOT_LIBRARY_DIR}/libEG.so)\n\
endif()                                         \n\n"
        
    towrite += "set( reqd_lib_output )\n"
    towrite += "set( exclude_lib_output )\n\n"
    
    # now link the shared library to their respective plugin libraries
    for i in xrange(len(plug_type)):
        directories = [ name for name in os.listdir(src_paths[i]) if os.path.isdir(src_paths[i] + "/" + name) ]
        for directory in sorted(directories):
                
            towrite += "#################### lib" + plug_type[i] + "_" + directory + ".so ####################\n\n"
                
            towrite += "set (" + plug_type[i] + "_compile_flag_" + directory + " \"\")\n"
            
            for plug in scanbit_plugins[plug_type[i]][directory]:
                if plug[3] == "excluded":
                    towrite += "set (" + plug_type[i] + "_compile_flag_" + directory + "\"    user: " + plug[4] + "\\n\")\n"
                
            towrite += "\n"
                
            towrite += "set (" + plug_type[i] + "_plugin_libraries_" + directory + "\n"
            if scanbit_libs.has_key(plug_type[i]):
                if scanbit_libs[plug_type[i]].has_key(directory):
                    towrite += " "*16 + "\"" + scanbit_links[plug_type[i]][directory] + "\"\n"
            towrite += ")\n\n"
            
            towrite += "set (" + plug_type[i] + "_plugin_lib_paths_" + directory + "\n"
            if scanbit_lib_hints.has_key(plug_type[i]):
                if scanbit_lib_hints[plug_type[i]].has_key(directory):
                    for lib in scanbit_lib_hints[plug_type[i]][directory]:
                        towrite += " "*16 + lib + "\n"
            towrite += ")\n\n"
            
            towrite += "set (" + plug_type[i] + "_plugin_rpath_" + directory + "\n"
            if scanbit_libs.has_key(plug_type[i]):
                if scanbit_libs[plug_type[i]].has_key(directory):
                    unique_libdirs = set(p for p in scanbit_libs[plug_type[i]][directory])
                    if unique_libdirs:
                        for libdir in unique_libdirs:
                            towrite += " "*16 + libdir +"\n"
            towrite += ")\n\n"
            
            towrite += "set (" + plug_type[i] + "_plugin_includes_" + directory + "\n"
            towrite += " "*16 + "${PLUGIN_INCLUDE_DIRECTORIES}\n"
            towrite += " "*16 + "${CMAKE_CURRENT_SOURCE_DIR}/include/gambit/ScannerBit/" + plug_type[i] + "s/" + directory + "\n"
            if scanbit_incs.has_key(plug_type[i]):
                if scanbit_incs[plug_type[i]].has_key(directory):
                    for inc in scanbit_incs[plug_type[i]][directory]:
                        towrite += " "*16 + inc + "\n"
            towrite += ")\n\n"
            
            towrite += "set (" + plug_type[i] + "_plugin_linked_libs_" + directory + " \"\")\n"
            if scanbit_link_libs.has_key(plug_type[i]):
                if scanbit_link_libs[plug_type[i]].has_key(directory):
                    for lib in scanbit_link_libs[plug_type[i]][directory]:
                        towrite += "set (" + plug_type[i] + "_plugin_linked_libs_" + directory + " \"${" + plug_type[i] + "_plugin_linked_libs_" + directory + "}" 
                        towrite += "    " + lib[0] + ": " + lib[1] + "\\n\")\n"
            
            towrite += "set (" + plug_type[i] + "_plugin_found_incs_" + directory + " \"\")\n"
            if scanbit_incs.has_key(plug_type[i]):
                if scanbit_incs[plug_type[i]].has_key(directory):
                    for inc in scanbit_incs[plug_type[i]][directory]:
                        towrite += "set (" + plug_type[i] + "_plugin_found_incs_" + directory + " \"${" + plug_type[i] + "_plugin_found_incs_" + directory + "}" 
                        towrite += "    \\\"" + plug_type[i] + "_locations.yaml\\\": " + inc + "\\n\")\n"
            if scanbit_inc_files.has_key(plug_type[i]):
                if scanbit_inc_files[plug_type[i]].has_key(directory):
                    for inc in scanbit_inc_files[plug_type[i]][directory]:
                        towrite += "set (" + plug_type[i] + "_plugin_found_incs_" + directory + " \"${" + plug_type[i] + "_plugin_found_incs_" + directory + "}" 
                        towrite += "    \\\"" + inc[0] + "\\\": " + inc[1] + "\\n\")\n"
            
            towrite += "\n"
            
            if scanbit_auto_libs.has_key(plug_type[i]):
                if scanbit_auto_libs[plug_type[i]].has_key(directory):
                    if scanbit_link_libs.has_key(plug_type[i]):
                        if scanbit_link_libs[plug_type[i]].has_key(directory):
                            temp = [p[0] for p in scanbit_link_libs[plug_type[i]][directory]]
                            scanbit_auto_libs[plug_type[i]][directory] = set(lib for lib in scanbit_auto_libs[plug_type[i]][directory] if not lib in temp);
                    
                    temp = set(lib for lib in scanbit_auto_libs[plug_type[i]][directory])
                    for lib in temp:
                        if lib == "ROOT":
                            towrite += "if (" + lib + "_FOUND)\n"
                            towrite += " "*4 + "foreach (" + lib + "_LIB ${" + lib + "_LIBRARIES})\n"
                            towrite += " "*8 + "get_filename_component(lib_path ${" + lib + "_LIB} PATH)\n"
                            towrite += " "*8 + "get_filename_component(lib_name ${" + lib + "_LIB} NAME_WE)\n"
                            towrite += " "*8 + "string (REGEX REPLACE \"^lib\" \"\" lib_name ${lib_name})\n"
                            towrite += " "*8 + "set (" + plug_type[i] + "_plugin_libraries_" + directory
                            towrite += " \"${" + plug_type[i] + "_plugin_libraries_" + directory + "}"
                            towrite += " -L${lib_path} -l${lib_name}\")\n"
                            towrite += " "*8 + "set (" + plug_type[i] + "_plugin_linked_libs_" + directory 
                            towrite += " \"${" + plug_type[i] + "_plugin_linked_libs_" + directory +"}    " + lib + ": ${" + lib + "_LIB}\\n\")\n"
                            towrite += " "*4 + "endforeach()\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_rpath_" + directory
                            towrite += " \"${" + plug_type[i] + "_plugin_rpath_" + directory + "};${" + lib + "_LIBRARY_DIR}\")\n"
                            towrite += "endif()\n\n"
                        else:
                            lib_name = plug_type[i] + "_" + directory + "_" + lib + "_LIBRARY"
                            towrite += "find_library( " + lib_name + " " + lib + " HINTS ${" + plug_type[i] + "_plugin_lib_paths_" + directory + "} )\n"
                            towrite += "message(\"Found library? : ${" + lib_name + "}\")\n" #bjf> tmp 
                            towrite += "if( NOT " + lib_name + " STREQUAL \"" + lib_name + "-NOTFOUND\" )\n" 
                            towrite += " "*4 + "get_filename_component(lib_path ${" + lib_name + "} PATH)\n"
                            towrite += " "*4 + "get_filename_component(lib_name ${" + lib_name + "} NAME_WE)\n"
                            towrite += " "*4 + "string (REGEX REPLACE \"^lib\" \"\" lib_name ${lib_name})\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_libraries_" + directory
                            towrite += " \"${" + plug_type[i] + "_plugin_libraries_" + directory + "}"
                            towrite += " -L${lib_path} -l${lib_name}\")\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_rpath_" + directory
                            towrite += " \"${" + plug_type[i] + "_plugin_rpath_" + directory + "};${lib_path}\")\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_linked_libs_" + directory 
                            towrite += " \"${" + plug_type[i] + "_plugin_linked_libs_" + directory +"}    " + lib + ": ${" + lib_name + "}\\n\")\n"
                            towrite += "endif()\n\n"
            
            if scanbit_auto_incs.has_key(plug_type[i]):
                if scanbit_auto_incs[plug_type[i]].has_key(directory):
                    if scanbit_inc_files.has_key(plug_type[i]):
                        if scanbit_inc_files[plug_type[i]].has_key(directory):
                            temp = [p[0] for p in scanbit_inc_files[plug_type[i]][directory]]
                            scanbit_auto_incs[plug_type[i]][directory] = set(inc for inc in scanbit_auto_incs[plug_type[i]][directory] if not inc in temp);
                    
                    temp = set(inc for inc in scanbit_auto_incs[plug_type[i]][directory])
                    for inc in temp:
                        if inc == "ROOT":
                            towrite += "if (" + inc + "_FOUND)\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_includes_" + directory + "\n"
                            towrite += " "*8 + "${" + plug_type[i] + "_plugin_includes_" + directory + "}\n"
                            towrite += " "*8 + "${ROOT_INCLUDE_DIR}\n"
                            towrite += " "*4 + ")\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_found_incs_" + directory
                            towrite += " \"${" +  plug_type[i] + "_plugin_found_incs_" + directory + "}"
                            towrite += "    \\\"" + inc + "\\\": ${ROOT_INCLUDE_DIR}\\n\")\n"
                            towrite += "endif()\n\n"
                        else:
                            inc_name = plug_type[i] + "_" + directory + "_" + re.sub(r";|/|\.", "_", inc) + "_INCLUDE_PATH"
                            towrite += "find_path( " + inc_name + " \"" + inc + "\" HINTS ${" + plug_type[i] + "_plugin_includes_" + directory + "})\n"
                            towrite += "if( NOT " + inc_name + " STREQUAL \"" + inc_name + "-NOTFOUND\" )\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_includes_" + directory + "\n"
                            towrite += " "*8 + "${" + plug_type[i] + "_plugin_includes_" + directory + "}\n"
                            towrite += " "*8 + "${" + inc_name + "}\n"
                            towrite += " "*4 + ")\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_plugin_found_incs_" + directory
                            towrite += " \"${" +  plug_type[i] + "_plugin_found_incs_" + directory + "}"
                            towrite += "    \\\"" + inc + "\\\": ${" + inc_name + "}\\n\")\n"
                            towrite += "else()\n"
                            towrite += " "*4 + "set (" + plug_type[i] + "_compile_flag_" + directory + " \"    file_missing: \\\"" + inc + "\\\"\")\n"
                            towrite += "endif()\n\n"
            towrite += "if( NOT ${" + plug_type[i] + "_plugin_linked_libs_" + directory + "} STREQUAL \"\" OR NOT ${" + plug_type[i] + "_plugin_found_incs_" + directory + "} STREQUAL \"\")\n"
            towrite += " "*4 + "set ( reqd_lib_output \"${reqd_lib_output}lib" + plug_type[i] + "_" + directory + ".so:\\n\" )\n"
            towrite += " "*4 + "if( NOT ${" + plug_type[i] + "_plugin_linked_libs_" + directory + "} STREQUAL \"\" )\n"
            towrite += " "*8 + "set ( reqd_lib_output \"${reqd_lib_output}  linked_libs: \\n${" + plug_type[i] + "_plugin_linked_libs_" + directory + "}\")\n"            
            towrite += " "*4 + "endif()\n"
            towrite += " "*4 + "if( NOT ${" + plug_type[i] + "_plugin_found_incs_" + directory + "} STREQUAL \"\" )\n"
            towrite += " "*8 + "set ( reqd_lib_output \"${reqd_lib_output}  found_incs: \\n${" + plug_type[i] + "_plugin_found_incs_" + directory + "}\")\n"
            towrite += " "*4 + "endif()\n"
            towrite += "endif()\n\n"
            
            towrite += "if ( " + plug_type[i] + "_compile_flag_" + directory + " STREQUAL \"\" )\n"
            towrite += " "*4 + "add_gambit_library( " + plug_type[i] + "_" + directory + " OPTION SHARED SOURCES ${" 
            towrite += plug_type[i] + "_plugin_sources_" + directory + "} HEADERS ${"
            towrite += plug_type[i] + "_plugin_headers_" + directory + "} )\n"
            towrite += " "*4 + "set_target_properties( " + plug_type[i] + "_" + directory + "\n" + " "*23 + "PROPERTIES\n"
            if sys.platform == "darwin":
                towrite += " "*23 + "LINK_FLAGS \"-dynamiclib ${" + plug_type[i] + "_plugin_libraries_" + directory + "}\"\n"
            else:
                towrite += " "*23 + "LINK_FLAGS \"-rdynamic ${" + plug_type[i] + "_plugin_libraries_" + directory + "}\"\n"
            towrite += " "*23 + "INSTALL_RPATH \"${" + plug_type[i] + "_plugin_rpath_" + directory + "}\"\n";
            if sys.platform == "darwin":
                cflags = "-dynamiclib"
            else:
                cflags = "-rdynamic"
            #if scanbit_static_links.has_key(plug_type[i]):
            #    if scanbit_static_links[plug_type[i]].has_key(directory):
            #        if (len(scanbit_static_links[plug_type[i]][directory]) != 0):
            #            cflags = "-static " + scanbit_static_links[plug_type[i]][directory]

            if cflags != "":
                towrite += " "*23 + "COMPILE_FLAGS \"" + cflags + "\"\n"
            towrite += " "*23 + "INCLUDE_DIRECTORIES \"${" + plug_type[i] + "_plugin_includes_" + directory + "}\"\n"
            towrite += " "*23 + "ARCHIVE_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/lib\"\n"
            towrite += " "*23 + "LIBRARY_OUTPUT_DIRECTORY \"${CMAKE_CURRENT_SOURCE_DIR}/lib\")\n"
            #towrite += "target_include_directories( " + inc_dirs ")\n\n"
            towrite += "else()\n"
            towrite += " "*4 + "set ( exclude_lib_output \"${exclude_lib_output}lib" + plug_type[i] + "_" + directory + ".so:\\n"
            towrite += "  plugins:\\n"
            for plug in scanbit_plugins[plug_type[i]][directory]:
                towrite += "    - " + plug[4] + "\\n"
            towrite += "  reason: \\n${" + plug_type[i] + "_compile_flag_" + directory + "}\\n\" )\n"
            towrite += "endif()\n\n"
            
    towrite += "file( WRITE ${PROJECT_SOURCE_DIR}/scratch/scanbit_excluded_libs.yaml \"${exclude_lib_output}\" )\n"
    towrite += "file( WRITE ${PROJECT_SOURCE_DIR}/scratch/scanbit_linked_libs.yaml \"${reqd_lib_output}\" )\n\n"

    cmake = "./ScannerBit/CMakeLists.txt"
    with open(cmake+".candidate","w") as f: f.write(towrite)
    update_cmakelists.update_only_if_different(cmake, cmake+".candidate")

    if verbose: print "Finished writing ScannerBit/CMakeLists.txt"

    towrite = "\
# GAMBIT: Global and Modular BSM Inference Tool  \n\
#************************************************\n\
# \\file                                         \n\
#                                                \n\
#  Cmake ScannerBit/reqd_entries.yaml for GAMBIT.\n\
#                                                \n\
#  This file has been automatically generated by \n\
#  locate_scanners.py.  Please do not modify.    \n\
#                                                \n\
#************************************************\n\
#                                                \n\
#  Authors:                                      \n\
#                                                \n\
#  \\author The GAMBIT Collaboration             \n\
#  \\date "+datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y")+"\n\
#                                                \n\
#************************************************\n\n"

    for type_key in scanbit_reqs:
        towrite += type_key + ":\n"
        for plug_key in scanbit_reqs[type_key]:
            towrite += " "*2 + plug_key + ":\n"
            for version_key in scanbit_reqs[type_key][plug_key]:
                towrite += " "*4 + version_key + ":\n"
                #if scanbit_reqs[type_key][plug_key][version_key][0] != "":
                towrite += " "*6 + "reqd_inifile_entries: [" + scanbit_reqs[type_key][plug_key][version_key][0] + "]\n"
                #if scanbit_reqs[type_key][plug_key][version_key][1] != "":
                towrite += " "*6 + "reqd_libraries: [" + scanbit_reqs[type_key][plug_key][version_key][1] + "]\n"
                towrite += " "*6 + "reqd_include_paths: [" + scanbit_reqs[type_key][plug_key][version_key][2] + "]\n"
                towrite += " "*6 + "linked_libraries: [" + ",".join(scanbit_reqs[type_key][plug_key][version_key][3]) + "]\n"
                towrite += " "*6 + "not_linked_libraries: [" + ",".join(scanbit_reqs[type_key][plug_key][version_key][4]) + "]\n"
                towrite += " "*6 + "found_include_paths: [" + ",".join(scanbit_reqs[type_key][plug_key][version_key][5]) + "]\n"
                towrite += " "*6 + "not_found_include_paths: [" + ",".join(scanbit_reqs[type_key][plug_key][version_key][6]) + "]\n"
        towrite += "\n"

    cmake = "./scratch/scanbit_reqd_entries.yaml"
    with open(cmake+".candidate","w") as f: f.write(towrite)
    update_cmakelists.update_only_if_different(cmake, cmake+".candidate")

    if verbose: print "Finished writing linkedout.cmake"

    towrite = "\
# GAMBIT: Global and Modular BSM Inference Tool  \n\
#************************************************\n\
# \\file                                         \n\
#                                                \n\
#  Cmake linking and rpath commands for GAMBIT.  \n\
#                                                \n\
#  This file has been automatically generated by \n\
#  locate_scanners.py.  Please do not modify.    \n\
#                                                \n\
#************************************************\n\
#                                                \n\
#  Authors:                                      \n\
#                                                \n\
#  \\author The GAMBIT Collaboration             \n\
#  \\date "+datetime.datetime.now().strftime("%I:%M%p on %B %d, %Y")+"\n\
#                                                \n\
#************************************************\n\
                                                 \n\
if (${CMAKE_SYSTEM_NAME} MATCHES \"Darwin\")     \n"
                                               
    if static_links != "":
        towrite += " "*4 + "foreach(program ${uses_scannerbit})\n"
        towrite += " "*8 + "target_link_libraries( ${program} -Wl,-force_load " + static_links + ")\n"
        towrite += " "*4 + "endforeach()\n"
    towrite += "else()\n"
    if static_links != "":
        towrite += " "*4 + "foreach(program ${uses_scannerbit})\n"
        towrite += " "*8 + "target_link_libraries( ${program} -Wl,--whole-archive " + static_links + " -Wl,--no-whole-archive )\n"
        towrite += " "*4 + "endforeach()\n"
    towrite += "endif()\n"

    cmake = "./cmake/linkedout.cmake"
    with open(cmake+".candidate","w") as f: f.write(towrite)
    update_cmakelists.update_only_if_different(cmake, cmake+".candidate")

    if verbose: print "Finished writing linkedout.cmake"

# Handle command line arguments (verbosity)
if __name__ == "__main__":
   main(sys.argv[1:])

