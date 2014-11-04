import json
from pprint import pprint
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
	answer_data=open('answers.json')
	output = json.load(output_data)
	answers = json.load(answer_data)
	outputLibraryFunctions = output['library_functions']
	answerLibraryFunctions = answers['library_functions']
	assert (len(outputLibraryFunctions) == len(answerLibraryFunctions))
	numWrongAnswers = 0
	numWrongDueToIIGlue = 0
	numFalsePositives = 0
	falsePositives = ''
	falseNegatives = ''
	numVarargs = 0
	numFalsePosDueToLength = 0
	numFalseNegsPassedToVararg = 0
	for i in range(0, len(outputLibraryFunctions)):
		outputFunc = outputLibraryFunctions[i]
		answerFunc = answerLibraryFunctions[i]
		assert(outputFunc['name'] == answerFunc['name'])
		outputAnnotations = outputFunc['argument_annotations']
		outputIIGlueAnnotations = outputFunc['args_array_receivers']
		answerAnnotations = answerFunc['argument_annotations']
		answerIIGlueAnnotations = answerFunc['args_array_receivers']
		for j in range (0, len(outputAnnotations)):
			if answerAnnotations[j] == 4 or answerFunc['args_array_receivers'][j] == 4:
				numVarargs += 1
			if answerAnnotations[j] == outputAnnotations[j]:
				continue
			if (answerAnnotations[j] == 2 or answerAnnotations[j] == 0) and answerAnnotations[j] != outputAnnotations[j]:
				numWrongAnswers += 1
				if outputIIGlueAnnotations[j] == 0:
					numWrongDueToIIGlue += 1
				if outputAnnotations[j] == 2:
					print outputFunc['name'] + "[" + str(j) + "] should be " + str(answerAnnotations[j]) + " found " + str(outputAnnotations[j]) + "\n"
					numFalsePositives += 1
					falsePositives += outputFunc['name'] + "[" + str(j) + "] should be " + str(answerAnnotations[j]) + " found " + str(outputAnnotations[j]) + "\n"
				elif outputAnnotations[j] == 0:
					falseNegatives += outputFunc['name'] + "[" + str(j) + "] should be " + str(answerAnnotations[j]) + " found " + str(outputAnnotations[j]) + "\n"
			elif answerAnnotations[j] == 1 and outputAnnotations[j] == 2:
				print "THIS SHOULD NOT HAPPEN YET\n"
				assert(False)
			elif answerAnnotations[j] == 3 and outputAnnotations[j] != 0 and outputAnnotations[j] != 3:
				numWrongAnswers += 1
				numFalsePositives += 1
				falsePositives += outputFunc['name'] + "[" + str(j) + "] should be " + str(answerAnnotations[j]) + " found " + str(outputAnnotations[j]) + "\n"
			
			elif answerAnnotations[j] == 5 and outputAnnotations[j] != 0 and outputAnnotations[j] != 5:
				numWrongAnswers += 1
				numFalsePosDueToLength += 1
				numFalsePositives += 1
				falsePositives += outputFunc['name'] + "[" + str(j) + "] should be " + str(answerAnnotations[j]) + " found " + str(outputAnnotations[j]) + "\n"
			elif answerAnnotations[j] == 6 and outputAnnotations[j] != 6 and outputAnnotations[j] != 2:
				numWrongAnswers += 1
				numFalseNegsPassedToVararg += 1
				falseNegatives += outputFunc['name'] + "[" + str(j) + "] should be " + str(answerAnnotations[j]) + " found " + str(outputAnnotations[j]) + "\n"
				
	print "Total number of functions: " + str(len(outputLibraryFunctions))
	print "Number of wrong answers: " + str(numWrongAnswers)
	print "Percent wrong answers: " + str(float(numWrongAnswers)/float(len(outputLibraryFunctions)))
	print "Number of wrong answers due to IIGlue: " + str(numWrongDueToIIGlue)
	print "Percent wrong answers due to IIGlue of all errors: " + str(float(numWrongDueToIIGlue)/float(numWrongAnswers))
	print "Percent wrong answers not due to IIGlue: " + str(float(numWrongAnswers-numWrongDueToIIGlue)/float(len(outputLibraryFunctions)))
	print "Number of false positives: " + str(numFalsePositives)
	print "Percent false positives: " + str(float(numFalsePositives)/float(len(outputLibraryFunctions)))
	print "Percent false positives of all errors: " + str(float(numFalsePositives)/float(numWrongAnswers))
	print "Number of false positives due to extra length parameter: " + str(numFalsePosDueToLength)
	print "Percent false positives due to extra length parameter: " + str(float(numFalsePosDueToLength)/float(len(outputLibraryFunctions)))
	print "Percent false positives due to extra length parameter of false positives: " + str(float(numFalsePosDueToLength)/float(numFalsePositives))
	print "Number of functions with varargs found: " + str(numVarargs)
	print "Percent of functions with varags found: " + str(float(numVarargs)/float(len(outputLibraryFunctions)))
	print "Number of false negatives passed to varargs " + str(numFalseNegsPassedToVararg)
	if '-v' in sys.argv:
		print "False positives: " + falsePositives
		print "False negatives: " + falseNegatives
	output_data.close()