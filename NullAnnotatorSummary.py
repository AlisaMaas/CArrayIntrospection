import json
from pprint import pprint
'''
Script to compute statistics about the output from NullAnnotator. Note that as of right now,
the answers.json file doesn't contain information about varargs, which is another source of 
false negatives, and also doesn't always annotate as null-terminated things which are only 
inferable as null-terminated because they are passed into a function as a varargs parameter.
When I have a way to reason about varargs parameters, hopefully this will be resolved.
'''
if __name__ == '__main__':
	output_data=open('output.json')
	answer_data=open('answers.json')
	output = json.load(output_data)
	answers = json.load(answer_data)
	outputLibraryFunctions = output['library_functions']
	answerLibraryFunctions = answers['library_functions']
	assert (len(outputLibraryFunctions) == len(answerLibraryFunctions))
	numWrongAnswers = 0
	numWrongDueToIIGlue = 0
	numFalsePositives = 0
	for i in range(0, len(outputLibraryFunctions)):
		outputFunc = outputLibraryFunctions[i]
		answerFunc = answerLibraryFunctions[i]
		assert(outputFunc['name'] == answerFunc['name'])
		outputAnnotations = outputFunc['argument_annotations']
		outputIIGlueAnnotations = outputFunc['args_array_receivers']
		answerAnnotations = answerFunc['argument_annotations']
		answerIIGlueAnnotations = answerFunc['args_array_receivers']
		for j in range (0, len(outputAnnotations)):
			if answerAnnotations[j] != outputAnnotations[j]:
				numWrongAnswers += 1
				if outputIIGlueAnnotations[j] == 0:
					numWrongDueToIIGlue += 1
				if outputAnnotations[j] > 0:
					numFalsePositives += 1
	print "Total number of functions: " + str(len(outputLibraryFunctions))
	print "Number of wrong answers: " + str(numWrongAnswers)
	print "Percent wrong answers: " + str(float(numWrongAnswers)/float(len(outputLibraryFunctions)))
	print "Number of wrong answers due to IIGlue: " + str(numWrongDueToIIGlue)
	print "Percent wrong answers due to IIGlue of all errors: " + str(float(numWrongDueToIIGlue)/float(numWrongAnswers))
	print "Percent wrong answers not due to IIGlue: " + str(float(numWrongAnswers-numWrongDueToIIGlue)/float(len(outputLibraryFunctions)))
	print "Number of false positives: " + str(numFalsePositives)
	print "Percent false positives: " + str(float(numFalsePositives)/float(len(outputLibraryFunctions)))
	print "Percent false positives of all errors: " + str(float(numFalsePositives)/float(numWrongAnswers))

	output_data.close()