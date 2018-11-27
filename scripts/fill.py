import os

def main():

	number = 1
	opone = []
	optwo = []
	answermult = [[0 for i in range(100)] for j in range(100)]
	answeradd = [[0 for i in range(100)] for j in range(100)]
	answersub = [[0 for i in range(100)] for j in range(100)]
	answerdiv = [[0 for i in range(100)] for j in range(100)]
	answermod = [[0 for i in range(100)] for j in range(100)]

	for i in range(0, 100):
		opone.append(i)
		optwo.append(i)

	for i in range(0, 100):
		for j in range(0, 100):
			answermult[i][j] = opone[i] * optwo[j]
			with open("data.txt", "a") as myfile:
				myfile.write("INSERT INTO QuestionInfo (questionNumber, op_one, operator, op_two, answer) VALUES (" + str(number) + ", " + str(opone[i]) + ", '*' , " + str(optwo[j]) + ", " + str(answermult[i][j]) + ");\n")
				number = number + 1

	for i in range(0, 100):
		for j in range(0, 100):
			answeradd[i][j] = opone[i] + optwo[j]
			with open("data.txt", "a") as myfile:
				myfile.write("INSERT INTO QuestionInfo (questionNumber, op_one, operator, op_two, answer) VALUES (" + str(number) + ", " + str(opone[i]) + ", '+' , " + str(optwo[j]) + ", " + str(answeradd[i][j]) + ");\n")
				number = number + 1

	for i in range(0, 100):
		for j in range(0, 100):
			answersub[i][j] = opone[i] - optwo[j]
			with open("data.txt", "a") as myfile:
				myfile.write("INSERT INTO QuestionInfo (questionNumber, op_one, operator, op_two, answer) VALUES (" + str(number) + ", "  + str(opone[i]) + ", '-' , " + str(optwo[j]) + ", " + str(answersub[i][j]) + ");\n")
				number = number + 1

	for i in range(0, 100):
		for j in range(0, 100):
			if optwo[j] == 0:
				answerdiv[i][j] = "error"
			else:
				if ((opone[i] /  float(optwo[j])) % 1 == 0):
					answerdiv[i][j] = opone[i] / optwo[j]
					with open("data.txt", "a") as myfile:
						myfile.write("INSERT INTO QuestionInfo (questionNumber, op_one, operator, op_two, answer) VALUES (" + str(number) + ", "  + str(opone[i]) + ", '/' , " + str(optwo[j]) + ", " + str(answerdiv[i][j]) + ");\n")
						number = number + 1
    
	for i in range(0, 100):
		for j in range(0, 100):
			if optwo[j] == 0:
				answermod[i][j] = "error"
			else:
				answermod[i][j] = opone[i] % optwo[j]
				with open("data.txt", "a") as myfile:
					myfile.write("INSERT INTO QuestionInfo (questionNumber, op_one, operator, op_two, answer) VALUES (" + str(number) + ", "  + str(opone[i]) + ", '%' , " + str(optwo[j]) + ", " + str(answermod[i][j]) + ");\n")
					number = number + 1
		
if __name__ == "__main__":
	main()
