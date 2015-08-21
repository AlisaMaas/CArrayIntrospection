#!/usr/bin/python

import json
from pprint import pprint
import sys
class Result:
    def __init__(self, functionName, argNumber, argName, len, reason, metadata):
        self.functionName = functionName
        self.argNumber = argNumber
        self.argName = argName
        self.len = len
        self.metadata = metadata
        self.reason = reason
    def __eq__(self, other):
        return other.functionName == self.functionName and other.argNumber == self.argNumber
    def __ne__(self, other):
        return not self.__eq__(other)
    def __hash__(self):
        return self.functionName.__hash__() + self.argNumber

def printDifferences(actualPositives, actualNegatives, observedPositives, observedNegatives):
    TP = len(actualPositives & observedPositives)
    TN = len(actualNegatives & observedNegatives)
    FP = len(observedPositives - actualPositives)
    FN = len(observedNegatives - actualNegatives)
    print "\tTrue Pos: " + str(TP)
    print "\tTrue Neg: " + str(TN)
    print "\tFalse Pos: " + str(FP)
    print "\tFalse Neg: " + str(FN)
    print "\tProportion of arguments correct: " + str(float(TP + TN)/(FP + FN + TP + TN))
    print "\tProportion of false negatives: " + str(float(FN)/(FN + TP))
    print "\tProportion of true positives: " + str(float(TP)/(TP + FN))
    print "\tProportion of true negatives: " + str(float(TN)/(TN + FP))
    print "\tProportion of false positives: " + str(float(FP)/(FP + TN))
    print "\t\tFalse positives: "
    for result in observedPositives - actualPositives:
        print "\t\t\t" + str(result.functionName) + " with argument number " + str(result.argNumber) + " (" + str(result.argName) + ")"
        print "\t\t\tmetadata: " + str(result.metadata)
    print "\t\tFalse negatives: "
    for result in observedNegatives - actualNegatives:
        print "\t\t\t" + str(result.functionName) + " with argument number " + str(result.argNumber) + " (" + str(result.argName) + ")"
        print "\t\t\tmetadata: " + str(result.metadata)
def analyzeReport(report):
    fixedLen = set()
    notFixedLen = set()
    symbolicLen = set()
    notSymbolicLen = set()
    sentinelTerm = set()
    notSentinelTerm = set()
    everything = set()
    functionCount = 0
    for function in report:
        if "metadata" in function and "dependency" in function['metadata']:
            continue # skip all dependencies
        i = 0
        functionCount += 1
        functionName = function['function_name']
        for argument in function['arguments']:
            metadata = []
            if 'metadata' in argument:
                metadata = argument['metadata']
            # if the length type is "other", it means it's none of the length types, and may be inconsistent
            argumentName = argument['argument_name']
            reasons = argument['argument_reason']
            lengthPairs = [("other", [notFixedLen, notSymbolicLen, notSentinelTerm]),
            ("symbolic", [notFixedLen, symbolicLen, notSentinelTerm]),
            ("sentinel", [notFixedLen, notSymbolicLen, sentinelTerm]),
            ("fixed", [fixedLen, notSymbolicLen, notSentinelTerm])]
            for (lengthType, lists) in lengthPairs:
                if lengthType in argument:
                    result = Result(functionName, i, argumentName, argument[lengthType], reasons, metadata)
                    for list in lists:
                        list.add(result)
                    everything.add(result)
            i += 1
    return (everything, fixedLen, notFixedLen, symbolicLen, notSymbolicLen, sentinelTerm, notSentinelTerm, functionCount)

'''
Script to compute statistics about the output from the Annotator. Note that as of right now,
the answers.json file doesn't contain information about varargs, which is another source of
false negatives, and also doesn't always annotate as null-terminated things which are only
inferable as null-terminated because they are passed into a function as a varargs parameter.
When I have a way to reason about varargs parameters, hopefully this will be resolved.
'''
if __name__ == '__main__':
    output_data = open(sys.argv[1])
    answer_data = open(sys.argv[2])
    output = json.load(output_data)
    answers = json.load(answer_data)
    outputLibraryFunctions = output['library_functions']
    answerLibraryFunctions = answers['library_functions']
    # statistics we care about go here.

    (totalEverything, totalFixedLen, totalNotFixedLen,
    totalSymbolicLen, totalNotSymbolicLen, totalSentinelTerm,
    totalNotSentinelTerm, functionCount) = analyzeReport(answerLibraryFunctions)

    (observedEverything, observedFixedLen, observedNotFixedLen,
    observedSymbolicLen, observedNotSymbolicLen,
    observedSentinelTerm, observedNotSentinelTerm, functionCount) = analyzeReport(outputLibraryFunctions)

    allCollections = [totalFixedLen, totalNotFixedLen, totalSymbolicLen, totalNotSymbolicLen,
    totalSentinelTerm, totalNotSentinelTerm, observedFixedLen, observedNotFixedLen, observedSymbolicLen,
    observedNotSymbolicLen, observedSentinelTerm, observedNotSentinelTerm]

    functionNames = set()
    for item in totalEverything - observedEverything:
        if item.functionName not in functionNames:
            print "Found " + item.functionName + " in answer library but not observed in output."
            functionNames.add(item.functionName)
        for collection in allCollections:
            if item in collection:
                collection.remove(item)
        totalEverything.remove(item)
    functionNames = set()
    for item in observedEverything - totalEverything:
        if item.functionName not in functionNames:
            print "Found " + item.functionName + " observed in output but not in answer library."
            functionNames.add(item.functionName)
        for collection in allCollections:
            if item in collection:
                collection.remove(item)
        observedEverything.remove(item)
    assert(len(totalEverything) == len(observedEverything))

    # output
    print "Number of functions: " + str(functionCount)
    print "Number of arguments: " + str(len(observedEverything))
    print "Sentinel terminated Info: "
    printDifferences(totalSentinelTerm, totalNotSentinelTerm, observedSentinelTerm, observedNotSentinelTerm)
    print "Symbolic Length Info: "
    printDifferences(totalSymbolicLen, totalNotSymbolicLen, observedSymbolicLen, observedNotSymbolicLen)
    print "Fixed Length Info: "
    printDifferences(totalFixedLen, totalNotFixedLen, observedFixedLen, observedNotFixedLen)
