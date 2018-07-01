//==========EXAMPLE USAGE OF MEDIATOR PATTERN LIBRARY =======================

#include <iostream>
#include "MediatorUtil.h"
/*
 * This example illustrates an office scenario.
 * In this office, there are different kind of Employee s as
 * Salesmen, Managers, System Administrators, Financial Managers and CEO
 * Two communities are identified like general Staff and Administration staff
 * SalesMen, Managers, SysAdmins & Finance Managers
 * are joined in general staff community.
 * Managers, Finance Managers and CEO belong to Administration community.
 * These two communities, General Staff & Administration Staff are defined by two
 * Event Argument types such as StaffMsg (for General Staff )
 * and AdminMsg (for Administration Staff).
 * For ease of event handling, a class named GeneralStaff is there from which the
 * SalesMen, Managers, SysAdmins and FinanceMangers are inherited from.
 * Employee is the root class which is inherited from
 * IColleague which is a must for all Colleagues.
 */
/*
namespace GoFExample
{
*/
    using namespace std;
    /*
     * Staff message will be used as an EventArgument and
     * All General Staff are expected to subscribe / publish this kind of Events
     */
    class StaffMsg
    {
        public:
            string msgName;
            string msgData;
            StaffMsg(string eName, string eData) :
                msgName(eName), msgData(eData)
        {
        }
    };

    /*
     * Admin message will be used as an EventArgument and
     * All Administration Staff are expected to subscribe / publish this kind of Events
     */
    class AdminMsg
    {
        public:
            string eventName;
            string eventTime;
            AdminMsg(string eName, string eTime) :
                eventName(eName), eventTime(eTime)
        {
        }
    };
    /*
     * Base class for all employees in the company
     * It is extended from abstract class IColleague to become
     * eligible to subscribe colleague events
     */
    class Employee: public GoFPatterns::IColleague
    {
        public:
            string title; //Designation of employee
            string name; // Employee name
            /*
             * Constructor to set Designation and name
             */
            Employee(string eTitle, string eName) :
                title(eTitle) // Set designation & name
                , name(eName)
        {
        }
            virtual ~Employee()
            {
            }

    };
    /*
     * General staff class handles the common events to which most -
     * of the employees are interested in.
     */
    class GeneralStaff: public Employee
    {
        protected:
            //Declare a colleague event which represent General Staff events.
            GoFPatterns::ColleagueEvent<StaffMsg> generalStaffEvent;
            //Join the General Staff community
        public:
            /*
             * Constructor for Initializing GeneralStaffEvent
             */
            GeneralStaff(string eTitle, string eName) :
                Employee(eTitle, eName) //Initialize title and name
                , generalStaffEvent(this, OnColleagueEvent)
                //Initialize GeneralStaff Colleague Event
        {
        }
            /*
             * Display details of the received event, its data,
             * its source and recipient context.
             */
            static void OnColleagueEvent(IColleague *source, StaffMsg data,
                                         IColleague* context)
            {
                Employee *srcCollegue = static_cast<Employee*> (source);
                Employee *ctxCollegue = static_cast<Employee*> (context);
                cout << endl << ctxCollegue->title 
                    << " - " << ctxCollegue->name
                    << " is notified by " 
                    << srcCollegue->title << " - "
                    << srcCollegue->name 
                    << " of STAFF Event " << data.msgName
                    << " with " << data.msgData;
            }
    };

    /*
     * Sales men is a general staff who receives all general staff notification
     * Now he does not raise any General Staff event.
     */
    class SalesMen: public GeneralStaff
    {
        public:
            SalesMen(string eName) :
                GeneralStaff("Sales Man", eName)
        {
        }
    };

    /*
     * Manager is a General staff by primary design.
     * Also he has an additional subscription to the Administration community.
     * His general staff events are handled from the base class itself.
     * But the Administration events are handled in this class itself to show -
     * better fine tuned response.
     */
    class Manager: public GeneralStaff
    {
        //Join the administration community
        GoFPatterns::ColleagueEvent<AdminMsg> adminEvent;
        public:
        Manager(string eName) :
            GeneralStaff("Manager", eName),
            adminEvent(this, OnAdminEvent)
            // Initialize adminEvent with Event Handler function name
        {
        }
        /*
         * Book a meeting room and notify all other General staff that
         * the meeting room is reserved by this Manager.
         */
        void BookMeetingRoom(string meetingRoomName)
        {
            //Fire a GeneralStaff Event that the meeting room is booked.
            generalStaffEvent.FireEvent(StaffMsg("Meeting Room Booking",
                                                 meetingRoomName));
        }
        /*
         * Handle an administration event notification..
         * Now it just prints the event details to screen.
         */
        static void OnAdminEvent(IColleague *source, AdminMsg data,
                                 IColleague* context)
        {
            Employee *srcCollegue = static_cast<Employee*> (source);
            Employee *ctxCollegue = static_cast<Employee*> (context);
            cout << endl << "Manager - " 
                << ctxCollegue->name << " is notified by "
                << srcCollegue->title 
                << " - " << srcCollegue->name
                << " of Admin Event " 
                << data.eventName << " @ "
                << data.eventTime;
        }
    };

    /*
     * SysAdmin is a General staff. He receives all GenaralStaff events
     * Further more, he notifies all General staff when there is a
     * Software updation is required.
     */
    class SysAdmin: public GeneralStaff
    {
        public:
            SysAdmin(string eName) :
                GeneralStaff("Sys Admin", eName)
        {
        }
            /*
             * Notify all General staff that the Software of each staff has to be updated.
             */
            void AdviceForSoftwareUpdate(string swName)
            {
                //Fire a GeneralStaff Event that the Software of each staff has to be updated.
                generalStaffEvent.FireEvent(StaffMsg("Software Update Advice", swName));
            }
    };

    /*
     * Finance Manager is a General staff by primary design.
     * Also he has an additional subscription to the Administration community.
     * His general staff events are handled from the base class itself.
     * But the Administration events are handled in this class itself to show -
     * better fine tuned response.
     */
    class FinanceManager: public GeneralStaff
    {
        //Join the administration community
        GoFPatterns::ColleagueEvent<AdminMsg> adminEvent;
        public:
        FinanceManager(string eName) :
            GeneralStaff("Finance Manager", eName), adminEvent(this, OnAdminEvent)
        {
        }
        /*
         * Finance manager can raise an event to all General staff
         * to request for the Income tax document.
         */
        void RequestForIncomeTaxDocument(string docName)
        {
            generalStaffEvent.FireEvent(StaffMsg("IT Doc Request", docName));
        }
        /*
         * Handle an administration event notification..
         * Now it just prints the event details to screen.
         */
        static void OnAdminEvent(IColleague *source, AdminMsg data,
                                 IColleague* context)
        {
            Employee *srcCollegue = static_cast<Employee*> (source);
            Employee *ctxCollegue = static_cast<Employee*> (context);
            cout << endl << "Finance Manager - " 
                << ctxCollegue->name
                << " is notified by " 
                << srcCollegue->title << " - "
                << srcCollegue->name 
                << " of Admin Event " << data.eventName
                << " @ " << data.eventTime;
        }
    };

    /*
     * CEO - is not a General staff so he does not receive the
     * General staff events like, Meeting room Request ,  -
     * Software Update request, IncomeTaxDocumentRequest etc
     * CEO Has joined in the Administration Community where -
     * Managers and Finance Manager are members of.
     * Also CEO can raise an Administration Event to hold a -
     * Marketing strategy discussion.
     */
    class CEO: public Employee
    {
        //Join the administration community
        GoFPatterns::ColleagueEvent<AdminMsg> adminEvent;

        public:
        CEO(string eName) :
            Employee("CEO", eName), adminEvent(this, OnAdminEvent)
        {
        }

        /*
         * Handle an administration event notification..
         * Now it just prints the event details to screen.
         */
        static void OnAdminEvent(IColleague *source, AdminMsg data,
                                 IColleague* context)
        {
            Employee *srcCollegue = static_cast<Employee*> (source);
            Employee *ctxCollegue = static_cast<Employee*> (context);
            cout << endl << "CEO- " 
                << ctxCollegue->name << " is notified by "
                << srcCollegue->title 
                << " - " << srcCollegue->name
                << " of Admin Event " 
                << data.eventName << " @ "
                << data.eventTime;
        }
        /*
         * Raise an Admin. Event to hold a Marketing Strategy discussion.
         */
        void HoldAdministrationMeeting(string agenda, string time)
        {
            // Fire and Admin Event to notify all other Administration community members.
            adminEvent.FireEvent(AdminMsg(agenda, time));
        }
    };

    /*
     * Program entry point; main() function
     */
    int main()
    {
        Manager mng1("Vivek"), mng2("Pradeep");
        SysAdmin sys1("Sony");
        SalesMen sl1("Biju"), sl2("Santhosh"), sl3("David");
        CEO ceo1("Ramesh");
        mng1.BookMeetingRoom("Zenith Hall");
        cout << endl
            << "===========================================================";
        FinanceManager fin1("Mahesh");
        sys1.AdviceForSoftwareUpdate("Win XP SP3");
        cout << endl
            << "===========================================================";
        fin1.RequestForIncomeTaxDocument("Form 12C");
        cout << endl
            << "===========================================================";
        ceo1.HoldAdministrationMeeting("European Marketing Plan",
                                       "Wednesday 4:00PM");
        cout << endl
            << "===========================================================";

        return 0;
    }
/*
} //End name space GoFExample
*/
