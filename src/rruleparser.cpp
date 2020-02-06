#include "uICAL/cppstl.h"
#include "uICAL/error.h"
#include "uICAL/rruleparser.h"
#include "uICAL/util.h"

namespace uICAL {
    RRuleParser::ptr RRuleParser::init(const std::string rrule, const DateTime dtstart) {
        return RRuleParser::ptr(new RRuleParser(rrule, dtstart));
    }

    RRuleParser::RRuleParser(const std::string rrule, const DateTime dtstart)
    : dtstart(dtstart)
    {
        this->freq = Freq::NONE;
        this->wkst = DateTime::Day::MON;
        this->interval = 1;
        this->count = -1;

        this->parseRRule(rrule);
    }

    void RRuleParser::parseRRule(const std::string rrule) {
        tokenize(rrule, ';', [&](const std::string part){
            size_t equals = part.find("=");
            const std::string key = part.substr(0, equals);
            const std::string value = part.substr(equals + 1, std::string::npos);

            if (key == "FREQ") {
                if      (value == "SECONDLY") this->freq = Freq::SECONDLY;
                else if (value == "MINUTELY") this->freq = Freq::MINUTELY;
                else if (value == "HOURLY") this->freq = Freq::HOURLY;
                else if (value == "DAILY") this->freq = Freq::DAILY;
                else if (value == "WEEKLY") this->freq = Freq::WEEKLY;
                else if (value == "MONTHLY") this->freq = Freq::MONTHLY;
                else if (value == "YEARLY") this->freq = Freq::YEARLY;
                else {
                    throw ParseError(std::string("Unknonwn RRULE:FREQ type: ") + value);
                }
            }
            else if (key == "WKST") {
                this->wkst = this->parseDay(value);
            }
            else if (key == "INTERVAL") {
                this->interval = this->parseInt(value);
            }
            else if (key == "UNTIL") {
                this->until = this->parseDate(value);
            }
            else if (key == "COUNT") {
                this->count = this->parseInt(value);
            }
            else if (key == "BYSECOND") {
                this->bySecond = toVector<unsigned>(value);
            }
            else if (key == "BYMINUTE") {
                this->byMinute = toVector<unsigned>(value);
            }
            else if (key == "BYHOUR") {
                this->byHour = toVector<unsigned>(value);
            }
            else if (key == "BYDAY") { // FREQ=MONTHLY;INTERVAL=2;COUNT=10;BYDAY=1SU,-1SU
                this->byDay = this->parseByDay(value);
            }
            else if (key == "BYWEEKNO") {
                this->byWeekNo = toVector<unsigned>(value);
            }
            else if (key == "BYMONTH") {
                this->byMonth = toVector<unsigned>(value);
            }
            else if (key == "BYMONTHDAY") {
                this->byMonthDay = toVector<int>(value);
            }
            else if (key == "BYYEARDAY") {
                this->byYearDay = toVector<int>(value);
            }
            else if (key == "BYSETPOS") {
                this->bySetPos = toVector<int>(value);
            }
            else {
                throw ParseError(std::string("Unknonwn RRULE key: ") + key);
            }
        });
    }

    int RRuleParser::parseInt(const std::string value) const {
        return string_to_int(value);
    }

    std::vector<std::string> RRuleParser::parseArray(const std::string value) const {
        std::vector<std::string> array;
        tokenize(value, ',', [&](const std::string part){
            array.push_back(part);
        });
        return array;
    }

    RRuleParser::Day_vector RRuleParser::parseByDay(const std::string value) const {
        Day_vector array;
        tokenize(value, ',', [&](const std::string part){
            int index;
            if (part.length() > 2) {
                index = string_to_int(part.substr(0, part.length() - 2));
            }
            else{
                index = 0;
            }
            
            DateTime::Day day = this->parseDay(part.substr(part.length() - 2, std::string::npos));
            array.push_back(Day_pair(index, day));
        });
        return array;
    }

    DateTime::Day RRuleParser::parseDay(const std::string value) const {
        if (value == "SU") return DateTime::Day::SUN;
        if (value == "MO") return DateTime::Day::MON;
        if (value == "TU") return DateTime::Day::TUE;
        if (value == "WE") return DateTime::Day::WED;
        if (value == "TH") return DateTime::Day::THU;
        if (value == "FR") return DateTime::Day::FRI;
        if (value == "SA") return DateTime::Day::SAT;
        throw ParseError(std::string("Unknown day name: ") + value);
    }

    DateTime RRuleParser::parseDate(const std::string value) const {
        return DateTime(value);
    }

    const char* RRuleParser::dayAsString(DateTime::Day day) const {
        switch(day) {
            case DateTime::Day::MON:    return "MO";
            case DateTime::Day::TUE:    return "TU";
            case DateTime::Day::WED:    return "WE";
            case DateTime::Day::THU:    return "TH";
            case DateTime::Day::FRI:    return "FR";
            case DateTime::Day::SAT:    return "SA";
            case DateTime::Day::SUN:    return "SU";
            default:    
                std::string err("Unknown day index: ");
                err += (int)day;
                throw ParseError(err);
        }
    }

    const char* RRuleParser::frequencyAsString(Freq freq) const {
        switch(freq) {
            case Freq::SECONDLY:  return "SECONDLY";
            case Freq::MINUTELY:  return "MINUTELY";
            case Freq::HOURLY:    return "HOURLY";
            case Freq::DAILY:     return "DAILY";
            case Freq::WEEKLY:    return "WEELKY";
            case Freq::MONTHLY:   return "MONTHLY";
            case Freq::YEARLY:    return "WEEKLY";
            default:
                std::string err("Unknown frequency index: ");
                err += (int)freq;
                throw ParseError(err);
        }
    }

    std::string RRuleParser::intAsString(int value) const {
        std::ostringstream out;
        out << value;
        return out.str();
    }

    void RRuleParser::exclude(const DateTime exclude) {
        this->excludes.push_back(exclude);
    }

    bool RRuleParser::excluded(const DateTime now) const {
        auto it = std::find(this->excludes.begin(), this->excludes.end(), now);
        if (it == this->excludes.end()) {
            return false;
        }
        return true;
    }

    std::string RRuleParser::str() const {
        std::ostringstream out;
        this->str(out);
        return out.str();
    }

    void RRuleParser::str(std::ostream& out) const {
        out << "RRULE:";

        Joiner values(';');
        
        values.out() << "FREQ=" << + this->frequencyAsString(this->freq);
        values.next();

        if (this->interval) {
            values.out() << "INTERVAL=" << this->intAsString(this->interval);
            values.next();
        }

        if (this->count > 0) {
            values.out() << "COUNT=" << this->intAsString(this->count);
            values.next();
        }

        values.out() << "WKST=" << this->dayAsString(this->wkst);
        values.next();

        if (this->byDay.size()) {
            Joiner days(',');
            for (Day_pair id : this->byDay) {
                if (id.first)
                    days.out() << id.first;
                days.out() << this->dayAsString(id.second);
                days.next();
            }

            values.out() << "BYDAY=" << days.str();
            values.next();
        }

        if (this->until.valid()) {
            values.out() << "UNTIL=" << this->until;
            values.next();
        }

        if (this->bySecond.size()) {
            values.out() << "BYSECOND=" << this->bySecond;
            values.next();
        }
        
        if (this->byMinute.size()) {
            values.out() << "BYMINUTE=" << this->byMinute;
            values.next();
        }
        
        if (this->byHour.size()) {
            values.out() << "BYHOUR=" << this->byHour;
            values.next();
        }

        if (this->byMonthDay.size()) {
            values.out() << "BYMONTHDAY=" << this->byMonthDay;
            values.next();
        }

        if (this->byMonth.size()) {
            values.out() << "BYMONTH=" << this->byMonth;
            values.next();
        }

        if (this->byYearDay.size()) {
            values.out() << "BYYEARDAY=" << this->byYearDay;
            values.next();
        }

        if (this->byWeekNo.size()) {
            values.out() << "BYWEEKNO=" << this->byWeekNo;
            values.next();
        }

        if (this->bySetPos.size()) {
            values.out() << "BYSETPOS=" << this->bySetPos;
            values.next();
        }

        values.str(out);
    }

    std::ostream & operator << (std::ostream &out, const RRuleParser::ptr &r) {
        r->str(out);
        return out;
    }

    std::ostream & operator << (std::ostream &out, const RRuleParser &r) {
        r.str(out);
        return out;
    }

    std::ostream & operator << (std::ostream &out, const RRuleParser::Day_pair &dp) {
        int idx;
        DateTime::Day day;
        unpack(dp, idx, day);
        if (idx)
            out << idx << day;
        else
            out << day;
        return out;
    }
}
