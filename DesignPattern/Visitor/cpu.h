//visit.h

#ifndef VISITOR_H
#define VISITOR_H

#include <iostream>
#include <string>
#include <vector>

class Element;
class CPU;
class VideoCard;
class MainBoard;

/*------------------*/
class Visitor {
    public:
        Visitor(std::string name) {
            visitorName = name;
        }
        virtual void visitCPU( CPU* cpu ) {};
        virtual void visitVideoCard( VideoCard* videoCard ) {};
        virtual void visitMainBoard( MainBoard* mainBoard ) {};


        std::string getName() {
            return this->visitorName;
        };
    private:
        std::string visitorName;
};


class Element {
    public:
        Element( std::string name ) {
            eleName = name;
        }
        virtual void accept( Visitor* visitor ) {};

        virtual std::string getName() {
            return this->eleName;
        }
    private:
        std::string eleName;
};

/*----------- Elements -------------*/

class CPU : public Element {
    public:
        CPU(std::string name) : Element(name) {}

        void accept(Visitor* visitor) {
            visitor->visitCPU(this);
        }
};

class VideoCard : public Element {
    public:
        VideoCard(std::string name) : Element(name) {}

        void accept(Visitor* visitor) {
            visitor->visitVideoCard(this);
        }
};

class MainBoard : public Element {
    public:
        MainBoard(std::string name) : Element(name) {}

        void accept(Visitor* visitor) {
            visitor->visitMainBoard(this);
        }
};

/*----------- ConcreteVisitor -------------*/

class CircuitDetector : public Visitor {
    public:
        CircuitDetector(std::string name) : Visitor(name) {}

        // checking cpu
        void visitCPU( CPU* cpu ) {
            std::cout << Visitor::getName() << " is checking CPU's circuits.(" << cpu->getName()<<")" << std::endl;
        }

        // checking videoCard
        void visitVideoCard( VideoCard* videoCard ) {
            std::cout << Visitor::getName() << " is checking VideoCard's circuits.(" << videoCard->getName()<<")" << std::endl;
        }

        // checking mainboard
        void visitMainBoard( MainBoard* mainboard ) {
            std::cout << Visitor::getName() << " is checking MainBoard's circuits.(" << mainboard->getName() <<")" << std::endl;
        }

};

class FunctionDetector : public Visitor {
    public:
        FunctionDetector(std::string name) : Visitor(name) {}
        virtual void visitCPU( CPU* cpu ) {
            std::cout << Visitor::getName() << " is check CPU's function.(" << cpu->getName() << ")"<< std::endl;
        }

        // checking videoCard
        void visitVideoCard( VideoCard* videoCard ) {
            std::cout << Visitor::getName() << " is checking VideoCard's function.(" << videoCard->getName()<< ")" << std::endl;
        }

        // checking mainboard
        void visitMainBoard( MainBoard* mainboard ) {
            std::cout << Visitor::getName() << " is checking MainBoard's function.(" << mainboard->getName() << ")"<< std::endl;
        }
};


/*------------------------*/

class Computer {
    public:
        Computer(CPU* cpu,
                 VideoCard* videocard,
                 MainBoard* mainboard) {
            elementList.push_back(cpu);
            elementList.push_back(videocard);
            elementList.push_back(mainboard);
        };
        void Accept(Visitor* visitor) {
            for( std::vector<Element*>::iterator i = elementList.begin(); i != elementList.end(); i++ )
            {
                (*i)->accept(visitor);
            }
        }; 
    private:
        std::vector<Element*> elementList;
};


#endif
