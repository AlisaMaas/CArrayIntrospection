import sys

if __name__ == "__main__":
  a = open(sys.argv[1])
  o = open(sys.argv[2])
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
  for i in a:
    answers.append(i.strip())
  for answer in answers:
    split = answer.split(",")
    if len(split[fixed]) > 0:
      print answer
  print numFixedLength
  for i in o:
    outputs.append(i.strip())
  i = 1
  j = 1
  while i < len(answers) and j < len(outputs):
    answer = answers[i].split(",")
    output = outputs[j].split(",")
    while answer[function_name] != output[function_name] or answer[slot] != output[slot]:
      if answer[function_name] != output[function_name]:
        if answer[function_name] < output[function_name]:
          i += 1
        else:
          j += 1
      elif answer[slot] < output[slot]:
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
    if answer[fixed] != output[fixed]:
      print answer[function_name] + " with argument #" + answer[slot] + " should be fixed length of " + answer[fixed] + " but saw " + output[fixed]
    if answer[sentinel] != output[sentinel]:
      print answer[function_name] + " with argument #" + answer[slot] + " should be sentinel terminated by " + answer[sentinel] + " but saw " + output[sentinel]
    if answer[symbolic] != output[symbolic]:
      print answer[function_name] + " with argument #" + answer[slot] + " should be symbolic length of " + answer[symbolic] + " but saw " + output[symbolic]
    j += 1  
    i += 1
  print "Num fixed length " + str(numFixedLength)
  print "Num sentinel terminated " + str(numSentinelTerminated)
  print "Num symbolic length " + str(numSymbolicLength)
  if numFixedLength != 0:
    print "True positive rate for fixed length: " + str(float(foundFixedLength)/float(numFixedLength))
  if numSentinelTerminated != 0:
    print "True positive rate for sentinel terminated: " + str(float(foundSentinelTerminated)/float(numSentinelTerminated))
  if numSymbolicLength != 0:
    print "True positive rate for symbolic length: " + str(float(foundSymbolicLength)/float(numSymbolicLength))
  print "Number of found symbolics: " + str(foundSymbolicLength)
  print "Number of false symbolic positives: " + str(falsePosSymbolic)
  print "Number of found sentinels: " + str(foundSentinelTerminated)
  print "Number of false sentinel positives: " + str(falsePosSentinel)
  print "Number of found fixed length: " + str(foundFixedLength)
  print "Number of false fixed positives: " + str(falsePosFixed)