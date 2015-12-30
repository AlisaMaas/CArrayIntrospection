import sys

'''
Usage: python ProduceResults.py name-of-answers-file.csv name-of-output-file.csv name-of-file-to-write.csv printable-name-of-library
'''
if __name__ == "__main__":
  a = open(sys.argv[1])
  o = open(sys.argv[2])
  r = open(sys.argv[3], 'w')
  name = sys.argv[4]
  if 'h' in sys.argv:
    human = True
  else:
    human = False
  answers = []
  outputs = []
  function_name = 0
  slot = 1
  fixed = 2
  sentinel = 3
  symbolic = 4
  numSentinelTerminated = 0
  numSymbolicLength = 0
  numFixedLength = 0
  foundSentinelTerminated = 0
  foundSymbolicLength = 0
  foundFixedLength = 0
  falsePosSentinel = 0
  falsePosSymbolic = 0
  falsePosFixed = 0
  functionNames = set()
  for i in a:
    answers.append(i.strip())
  '''for answer in answers:
    split = answer.split(",")
    if len(split[fixed]) > 0:
      print answer
  '''
 # print numFixedLength
  for i in o:
    outputs.append(i.strip())
  i = 1
  j = 1
  numArgs = 0
  while i < len(answers) and j < len(outputs):
    answer = answers[i].split(",")
    output = outputs[j].split(",")
    while answer[function_name] != output[function_name] or answer[slot] != output[slot]:
      if answer[function_name] != output[function_name]:
        if answer[function_name] < output[function_name]:
          i += 1
        else:
          j += 1
      elif int(answer[slot]) < int(output[slot]):
        i += 1
      else:
        j += 1
      if j >= len(outputs) or i >= len(answers):
        break
      output = outputs[j].split(",")
      answer = answers[i].split(",")
    if j >= len(outputs) or i >= len(answers):
      break
    assert (answer[function_name] == output[function_name])
    assert (answer[slot] == output[slot])
    functionNames.add(answer[function_name])
    if len(answer[fixed]) > 0:
      if answer[fixed] == output[fixed]:
        foundFixedLength += 1
      numFixedLength += 1
    if output[fixed] != answer[fixed] and len(output[fixed]) > 0:
        falsePosFixed += 1
    if len(answer[sentinel]) > 0 :
      if answer[sentinel] == output[sentinel]:
        foundSentinelTerminated += 1
      numSentinelTerminated += 1
    if output[sentinel] != answer[sentinel] and len(output[sentinel]) > 0:
        falsePosSentinel += 1
    if len(answer[symbolic]) > 0:
      if answer[symbolic] == output[symbolic]:
        foundSymbolicLength += 1
      numSymbolicLength += 1
    if answer[symbolic] != output[symbolic] and len(output[symbolic]) > 0:
        falsePosSymbolic += 1
   ''' if answer[fixed] != output[fixed]:
      print answer[function_name] + " with argument #" + answer[slot] + " should be fixed length of " + answer[fixed] + " but saw " + output[fixed]
    if answer[sentinel] != output[sentinel]:
      print answer[function_name] + " with argument #" + answer[slot] + " should be sentinel terminated by " + answer[sentinel] + " but saw " + output[sentinel]
    if answer[symbolic] != output[symbolic]:
      print answer[function_name] + " with argument #" + answer[slot] + " should be symbolic length of " + answer[symbolic] + " but saw " + output[symbolic]
    '''
    j += 1  
    i += 1
    numArgs += 1
  #Library details info, minus the KLoC and runtime
  #name,number-of-functions,number-of-all-args,number-of-symbolic-args,number-of-fixed-args,number-of-sentinel-args
  r.write(name + "," + str(len(functionNames)) + "," + str(numArgs) + "," + str(numSymbolicLength) + "," + str(numFixedLength) + "," + str(numSentinelTerminated))
  
  symbolicTPR = ""
  symbolicFPR = ""
  if numSymbolicLength != 0:
    symbolicTPR = str(float(foundSymbolicLength)/float(numSymbolicLength))
    symbolicFPR = float(falsePosSymbolic)/(falsePosSymbolic + (numArgs - numSymbolicLength))

  fixedTPR = ""
  fixedFPR = ""
  if numFixedLength != 0:
    fixedTPR = str(float(foundFixedLength)/float(numFixedLength))
    fixedFPR = float(falsePosFixed)/(falsePosFixed + (numArgs - numFixedLength))
  sentinelTPR = ""
  sentinelFPR = ""
  if numSentinelTerminated != 0:
    sentinelTPR = str(float(foundSentinelTerminated)/float(numSentinelTerminated))
    sentinelFPR = float(falsePosSentinel)/(falsePosSentinel + (numArgs - numSentinelTerminated))
  
  #name,symbolic-true-positive,symbolic-false-positive,fixed-true-positive,fixed-false-positive,sentinel-true-positive,sentinel-false-positive

  r.write("\n" + name + "," + str(symbolicTPR) + "," + str(symbolicFPR) + "," + str(fixedTPR) + "," + str(fixedFPR) + "," + str(sentinelTPR) + "," + str(sentinelFPR))