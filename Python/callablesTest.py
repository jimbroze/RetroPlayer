class TestClass:
    def __init__(self):
        return

    def testFunc1(self):
        print("hi")

    def testFunc2(self):
        print("bye")

    def testFunc3(self):
        print("Wut")

    def testFunc4(self):
        print("stahp")

testObj = TestClass()
testList = [[testObj.testFunc1, testObj.testFunc2], [testObj.testFunc3, testObj.testFunc4]]

testList[0][1]()