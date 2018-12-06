/*============================================================================
 * Name              : Mediator.h
 * Author           : Tojo Thomas tojohere@gmail.com
 * Description  : This file contains the definition of Mediator Library which
 *                                     consist of Mediator and ColleagueEvent classes
 * Date                 : 2009/04/01
 * Courtesy        : The concept of Componentizing GoF pattern is taken from
 *                               Patterns to Components [Doctoral thesis by Karine Arnout]
 *                               Concept of community based event broadcasting is self made.
 * Signature       : C++ programming using Eclipse and MinGW is a nice experience
 * ============================================================================
 */
#ifndef MEDIATOR_H_
#define MEDIATOR_H_
#include <vector>
#include <algorithm>
namespace GoFPatterns
{
    using namespace std;
    /*
     * All colleagues are expected to be inherited from this class.
     * Even though it has no functionality now, it is left as a -
     * place holder for future improvements.
     */
    class IColleague
    {
        protected:
            IColleague()
            {
            }
    };
    /*
     * Forward declaration of ColleagueEvent class
     */
    template<typename EventArgType>
        class ColleagueEvent;
    /*
     * Mediator class is a singleton class and its only one instance of it will be created for
     * each type of colleague collaboration.  An Instance of this class hold the reference to
     * all ColleagueEvent objects which fire/ receive same Event Argument Type.
     * Since this class is not meant to used public, it is exposed only to ColleagueEvent class as a friend.
     */
    template<typename EventArgType>
        class Mediator
        {
            typedef vector<ColleagueEvent<EventArgType>*> EventList;
            private:
            //List of ColleagueEvents in this community.
            EventList colleagues;

            static Mediator<EventArgType>* instance; //Singleton implementation
            /*
             * Access to the only one possible type specific object of this class.
             */
            static Mediator<EventArgType>& GetInstance()
            {
                if (!instance)
                {
                    instance = new Mediator();
                }
                return *instance;
            }
            /*
             * Register a ColleagueEvent in the list of colleagues in this community.
             */
            void RegisterCollegue(ColleagueEvent<EventArgType> *colEvent)
            {
                colleagues.push_back(colEvent);
            }
            /*
             * When a ColleagueEvent is fired, notify all other Events in this community
             */
            void FireEvent(ColleagueEvent<EventArgType> *source, EventArgType eventArg)
            {
                for (unsigned int i = 0; i < colleagues.size(); i++)
                {
                    //Notify all Events other than the one who fired the event.
                    if (colleagues[i] != source)
                    {
                        colleagues[i]->handlerProc(source->eventContext, eventArg,
                                                   colleagues[i]->eventContext);
                    }
                }
            }
            /*
             * Remove a ColleagueEvent from the list of colleagues in this community.
             */
            void UnregisterCollegue(ColleagueEvent<EventArgType> *colEvent)
            {
                typename EventList::iterator itr = find(colleagues.begin(),
                                                        colleagues.end(), colEvent);
                if (itr != colleagues.end())
                {
                    colleagues.erase(itr);
                }
            }
            friend class ColleagueEvent<EventArgType> ;
        };

    /*
     * The CollegueEvent template class is instantiated by means of an EventArgumentType
     * When an object is created, it registers itself with Mediator matching its EventArgAtype
     * Thus it becomes a member of the community of ColleagueEvents with same EventArgType
     * When an ColleagueEvent object is fired, it is informed to the community Mediator
     * And Mediator broadcast the event to all other CollegueEvents in that community.
     */
    template<typename EventArgType>
        class ColleagueEvent
        {
            typedef void (*ColleagueEventHandler)(IColleague *source, EventArgType eventArg, IColleague* context);
            IColleague * eventContext; //Context colleague who fires the event
            ColleagueEventHandler handlerProc; //Event handler delegate (function pointer)
            public:
            /*
             * Constructor receives the event source context and the EventHandler delegate
             * Also register this object with the Mediator to join the community.
             */
            ColleagueEvent(IColleague *source, ColleagueEventHandler eventProc) :
                eventContext(source),
                handlerProc(eventProc)
            {
                //Register with mediator
                Mediator<EventArgType>::GetInstance().RegisterCollegue(this);
            }
            /*
             * Destructor - unregister the object from community.
             */
            virtual ~ColleagueEvent()
            {
                Mediator<EventArgType>::GetInstance().UnregisterCollegue(this);
            }
            /*
             * FireEvent - Inform the mediator that the event object is triggered.
             * The mediator then will inform this to all other Events in this community.
             */
            void FireEvent(EventArgType eventArg)
            {
                Mediator<EventArgType>::GetInstance().FireEvent(this, eventArg);
            }
            friend class Mediator<EventArgType> ;
        };
    template<typename EventArgType>
        Mediator<EventArgType>* Mediator<EventArgType>::instance = 0; //Define the static member of Mediator class
} //namespace GoFPatterns
#endif /* MEDIATOR_H_ */

