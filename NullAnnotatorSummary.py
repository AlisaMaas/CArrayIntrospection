#!/usr/bin/python

import json
import sys
'''
Script to compute statistics about the output from NullAnnotator. Note that as of right now,
the answers.json file doesn't contain information about varargs, which is another source of
false negatives, and also doesn't always annotate as null-terminated things which are only
inferable as null-terminated because they are passed into a function as a varargs parameter.
When I have a way to reason about varargs parameters, hopefully this will be resolved.
'''
if __name__ == '__main__':
    output_data=open(sys.argv[1])
    answer_data=open(sys.argv[2])
    output = json.load(output_data)
    answers = json.load(answer_data)
    outputLibraryFunctions = output['library_functions']
    answerLibraryFunctions = answers['library_functions']

    outputKeySet = set(outputLibraryFunctions.keys())
    answerKeySet = set(answerLibraryFunctions.keys())
    for key in outputKeySet - answerKeySet:
        print 'output key not among answer keys:', key
    for key in answerKeySet - outputKeySet:
        print 'answer key not among output keys:', key
    #assert outputKeySet == answerKeySet
        
    functionNames = outputLibraryFunctions.keys()
    numWrongAnswers = 0
    numWrongDueToIIGlue = 0
    numFalsePositives = 0
    falsePositives = []
    falseNegatives = []
    numVarargs = 0
    numFalsePosDueToLength = 0
    numFalseNegsPassedToVararg = 0
    numArguments = 0
    numArrayArguments = 0
    numWrongArrays = 0
    numFalsePositiveArrays = 0
    numFalsePosDueToLengthArrays = 0
    numTruePositives = 0
    wronglyAnnotatedFunctions = set()
    for functionName in functionNames:
        outputFunc = outputLibraryFunctions[functionName]
        answerFunc = answerLibraryFunctions[functionName]
        outputAnnotations = outputFunc['argument_annotations']
        outputIIGlueAnnotations = outputFunc['args_array_receivers']
        answerAnnotations = answerFunc['argument_annotations']
        answerIIGlueAnnotations = answerFunc['args_array_receivers']
        if len(answerIIGlueAnnotations) > 0 and answerIIGlueAnnotations[0] == -1:
            continue
        for j in range(0, len(outputAnnotations)):
            if answerIIGlueAnnotations[j] == -1:
                continue; #skip anything that was a dependency, don't count it either way.
            if answerAnnotations[j] == 4 or answerFunc['args_array_receivers'][j] == 4:
                numVarargs += 1
                continue
            numArguments += 1
            if answerIIGlueAnnotations[j] == 1:
                numArrayArguments += 1
            if answerAnnotations[j] == outputAnnotations[j]:
                if answerAnnotations[j] == 2:
                    numTruePositives += 1
                continue

            def mismatch():
                return "%s[%s] (%s) should be %s found %s" % (
                    functionName, j, outputFunc['argument_names'][j],
                    answerAnnotations[j], outputAnnotations[j],
                )

            if (answerAnnotations[j] == 2 or answerAnnotations[j] == 0) and answerAnnotations[j] != outputAnnotations[j]:
                wronglyAnnotatedFunctions.add(functionName)
                numWrongAnswers += 1
                if answerIIGlueAnnotations[j] == 1:
                    numWrongArrays += 1
                if outputIIGlueAnnotations[j] == 0:
                    numWrongDueToIIGlue += 1
                if outputAnnotations[j] == 2:
                    print "%s because %s" % (mismatch(), outputFunc['argument_reasons'][j])
                    numFalsePositives += 1
                    if answerIIGlueAnnotations[j] == 1:
                        numFalsePositiveArrays += 1
                    falsePositives += mismatch()
                elif outputAnnotations[j] == 0:
                    falseNegatives += mismatch()
            elif answerAnnotations[j] == 1 and outputAnnotations[j] == 2:
                print "THIS SHOULD NOT HAPPEN YET\n"
                assert(False)
            elif answerAnnotations[j] == 3 and outputAnnotations[j] != 0 and outputAnnotations[j] != 3:
                wronglyAnnotatedFunctions.add(functionName)
                if answerIIGlueAnnotations[j] == 1:
                    numWrongArrays += 1
                    numFalsePositiveArrays += 1
                print "%s because %s" % (mismatch(), outputFunc['argument_reasons'][j])
                numWrongAnswers += 1
                numFalsePositives += 1
                falsePositives += mismatch()
            elif answerAnnotations[j] == 5 and outputAnnotations[j] != 0 and outputAnnotations[j] != 5:
                wronglyAnnotatedFunctions.add(functionName)
                if answerIIGlueAnnotations[j] == 1:
                    numWrongArrays += 1
                    numFalsePositiveArrays += 1
                    numFalsePosDueToLengthArrays += 1
                numWrongAnswers += 1
                numFalsePosDueToLength += 1
                numFalsePositives += 1
                falsePositives += mismatch()
            elif answerAnnotations[j] == 6 and outputAnnotations[j] != 6 and outputAnnotations[j] != 2:
                wronglyAnnotatedFunctions.add(functionName)
                if answerIIGlueAnnotations[j] == 1:
                    numWrongArrays += 1
                numWrongAnswers += 1
                numFalseNegsPassedToVararg += 1
                falseNegatives += mismatch()
                
    print "Total number of functions:", len(outputLibraryFunctions)
    print "Total number of arguments:", numArguments
    print "Total number of array arguments:", numArrayArguments
    print "Total number of wrongly annotated functions:", len(wronglyAnnotatedFunctions)
    print "Total percentage of wrongly annotated functions: %.1f%%" % (100. * len(wronglyAnnotatedFunctions) / len(outputLibraryFunctions))
    print "Number of wrong answers:", numWrongAnswers
    print "Percent wrong answers total: %.1f%%" % (100. * numWrongAnswers / numArguments)
    print "Percent wrong answers in array arguments: %.1f%%" % (100. * numWrongArrays / numArrayArguments)
    print "Number of wrong answers due to IIGlue:", numWrongDueToIIGlue
    if numWrongAnswers:
        print "Percent wrong answers due to IIGlue of all errors: %.1f%%" % (100. * numWrongDueToIIGlue / numWrongAnswers)
    print "Number of false positives:", numFalsePositives
    print "Number of true positives:", numTruePositives
    if numFalsePositives > 0:
        print "Percent false positives total: %.1f%%" % (100. * numFalsePositives / numArguments)
        print "Percent false positives of array arguments: %.1f%%" % (100. * numFalsePositiveArrays / numArrayArguments)
        print "Percent false positives of all errors: %.1f%%" % (100. * numFalsePositives / numWrongAnswers)
        print "Number of false positives due to extra length parameter:", numFalsePosDueToLength
        print "Percent false positives due to extra length parameter total: %.1f%%" % (100. * numFalsePosDueToLength / numArguments)
        print "Percent false positives due to extra length parameter of array arguments: %.1f%%" % (100. * numFalsePosDueToLengthArrays / numArrayArguments)
        print "Percent false positives due to extra length parameter of false positives: %.1f%%" % (100. * numFalsePosDueToLength / numFalsePositives)
    print "Number of functions with varargs found:", numVarargs
    print "Percent of functions with varags found: %.1f%%" % (100. * numVarargs / len(outputLibraryFunctions))
    print "Number of false negatives passed to varargs:", numFalseNegsPassedToVararg
    if '-v' in sys.argv:
        print "False positives:", "\n".join(falsePositives)
        print "False negatives:", "\n".join(falseNegatives)

# Local variables:
# indent-tabs-mode: t
# End:
