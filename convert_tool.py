#!/usr/bin/python

import json
from pprint import pprint
import sys

if __name__ == '__main__':
    file = open(sys.argv[1])
    original = json.load(file)
    original = original["library_functions"]
    functionNames = original.keys()
    functions = []
    
    for functionName in functionNames:
        argumentAnnotations = original[functionName]["argument_annotations"]
        functionDict = {}
        functionDict["function_name"] = functionName
        functionDict["arguments"] = []
        for i in range(len(argumentAnnotations)):
            dict = {}
            function = original[functionName]
            if "args_array_receivers" in function:
                if function["args_array_receivers"][i] == -1:
                    functionDict["metadata"] = ["dependency"]
                    break
                dict["args_array_receiver"] = function["args_array_receivers"][i]
            else:
                functionDict["metadata"] = ["dependency"]
            if "argument_names" in function:
                dict["argument_name"] = function["argument_names"][i]
            else:
                dict["argument_name"] = "unknown"
            if "argument_reasons" in function:
                dict["argument_reason"] = function["argument_reasons"][i]
            else:
                dict["argument_reason"] = "unknown"
            dict["metadata"] = []
            if argumentAnnotations[i] == 9:
                dict["metadata"].append("string passed to varargs")
                dict["sentinel"] = "NUL"
            elif argumentAnnotations[i] == 5:
                dict["metadata"].append("extra length parameter")
                dict["sentinel"] = "NUL"
            elif argumentAnnotations[i] == 2:
                dict["sentinel"] = "NUL"
            elif argumentAnnotations[i] == 8:
                dict["metadata"].append("complex analysis needed")
                dict["symbolic"] = function["symbolic_length"][i]
                assert(dict["symbolic"] != -1)
            elif argumentAnnotations[i] == 6:
                dict["symbolic"] = function["symbolic_length"][i]
                assert(dict["symbolic"] != -1)
            elif argumentAnnotations[i] == 7:
                dict["fixed"] = function["fixed_length"][i]
                assert(dict["fixed"] != -1)
            else:
                dict["other"] = function["argument_annotations"][i]
            functionDict["arguments"].append(dict)
        functions.append(functionDict)
    new = {"library_functions":functions}            
    modified = open(sys.argv[2], "w")
    out = json.dumps(new, ensure_ascii=False, sort_keys=True, indent=4, separators=(',', ': '))
    modified.write(out)
    modified.close()
    file.close()