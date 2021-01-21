/*
    Autorzy:
    Paweł Ligęza 417710
    Grzegorz Zaleski 418494
    Damian Chańko 394127
*/
#ifndef FUNCTION_MAXIMA_H
#define FUNCTION_MAXIMA_H

#include <cstddef>
#include <map>
#include <set>
#include <memory>

// Wyjątek zgłaszany przy value_at.
class InvalidArg : public std::exception {
public:
    [[nodiscard]] const char *what() const noexcept override {
        return "invalid argument value";
    }
};

// Główna klasa
template<typename A, typename V>
class FunctionMaxima {
public:
    // Deklaracja klasy punktu.
    class point_type;

private:
    // Aliasy na używane struktury.
    using A_t = std::shared_ptr<A>;
    using V_t = std::shared_ptr<V>;
    using set_iterator = typename std::set<point_type>::iterator;

    // Aliasy na tworzenie shared pointerów.
#define MSA std::make_shared<A>
#define MSV std::make_shared<V>

    // Komparator mapy.
    struct cmp_map {
        bool operator()(const A_t &A_, const A_t &B) const {
            return (*A_) < (*B);
        }
    };

    // Struktura mapy.
    using map_t = std::map<A_t, V_t, cmp_map>;
    //Mapa reprezentująca funkcje.
    map_t function;

    // Komparator seta.
    struct cmp_default {
        bool operator()(const point_type &A_, const point_type &B) const {
            return (A_.arg() < B.arg())
                   || (!(A_.arg() < B.arg()) && !(B.arg() < A_.arg()) && A_.value() < B.value());
        }
    };

    // Struktura seta.
    using set_t = std::set<point_type, cmp_default>;
    // Set trzymający punkty.
    set_t points;

    // Komparator seta maksimów.
    struct cmp_max {
        bool operator()(const point_type &A_, const point_type &B) const {
            return B.value() < A_.value()
                   || (!(A_.value() < B.value()) && !(B.value() < A_.value()) && A_.arg() < B.arg());
        }
    };

    // Struktura seta maksimów.
    using set_max_t = std::set<point_type, cmp_max>;
    // Deklaracja seta maksimów.
    set_max_t maxima;

    // Funkcje pomocnicze:

    // Czy należy usunąć punkt z maksimów.
    bool erase_maxima(A_t const &a, V_t const &v) {
        auto found2 = maxima.find(point_type(a, v));
        return found2 != maxima.end() || false;

    }

    // Rozpatruje sytuacje gdy po prawej stronie wstawianego punktu jest stary element do wyrzucenia.
    bool checkAddRight(set_iterator current,  bool &erasePrev, bool &eraseNext) {
        auto prev = current;
        auto next = current;

        if (current != points.begin())
            --prev;

        next++;
        next++;
        if (next != points.end() && *next->val < *current->val)
            eraseNext = erase_maxima(next->argument, next->val);

        if ((current != points.begin()) && *prev->val < *current->val)
            erasePrev = erase_maxima(prev->argument, prev->val);

        if ((current == points.begin() || !(*current->val < *prev->val))
            && (next == points.end() || !(*current->val < *next->val))) {
            return true;
        }

        return false;
    }

    // Rozpatruje sytuacje gdy po lewej stronie wstawianego punktu jest stary element do wyrzucenia.
    bool checkAddLeft(set_iterator current, bool &erasePrev, bool &eraseNext) {
        auto prev = current;
        auto next = current;
        bool begin = false;

        --prev;
        if(prev == points.begin())
            begin = true;
        else
            prev--;
        next++;

        if (next != points.end() && *next->val < *current->val)
            eraseNext = erase_maxima(next->argument, next->val);

        if (!begin && *prev->val < *current->val)
            erasePrev = erase_maxima(prev->argument, prev->val);

        if ((begin || !(*current->val < *prev->val))
            && (next == points.end() || !(*current->val < *next->val))) {
            return true;
        }

        return false;
    }

    // Rozpatruje sytuacje gdy nie ma w secie starego elementu do wyrzucenia.
    bool checkAddCenter(set_iterator current,  bool &erasePrev, bool &eraseNext) {
        auto prev = current;
        auto next = current;

        if (current != points.begin())
            --prev;

        next++;

        if (next != points.end() && *next->val < *current->val)
            eraseNext = erase_maxima(next->argument, next->val);

        if ((current != points.begin()) && *prev->val < *current->val)
            erasePrev = erase_maxima(prev->argument, prev->val);

        if ((current == points.begin() || !(*current->val < *prev->val))
            && (next == points.end() || !(*current->val < *next->val))) {
            return true;
        }

        return false;
    }

    // Klasa do bezpiecznego dodawania elementów.
    class insertGuard {
    private:
        // Wskazniki do struktur.
        set_t *insert_set;
        set_max_t *insert_set_max;

        // Iteratory do rollbacka bo sa noexcept.
        set_iterator set_element;
        set_iterator max_left;
        set_iterator max_center;
        set_iterator max_right;

        // Informacja czy istnieją punkty do rollbacka.
        bool element_exist;
        bool left_exist;
        bool center_exist;
        bool right_exist;

        // Czy należy wykonać rollback.
        bool Rollback;

    public:
        // Konstruktor
        insertGuard() : insert_set(nullptr), insert_set_max(nullptr), element_exist(false),
                        left_exist(false), center_exist(false), right_exist(false), Rollback(false) {}

        //Kopia zablokowana.
        insertGuard(const insertGuard &) = delete;
        //Przypisanie zablokowane.
        insertGuard &operator=(const insertGuard &) = delete;

        // Destruktor rollbackuje jeżeli wystąpił błąd.
        ~insertGuard() {
            if (Rollback) {
                if (element_exist)
                    insert_set->erase(set_element);
                if (left_exist)
                    insert_set_max->erase(max_left);
                if (center_exist)
                    insert_set_max->erase(max_center);
                if (right_exist)
                    insert_set_max->erase(max_right);
            }
        }

        // Bezpieczne wstawianie punktu do seta.
        void insert_to_set(set_t *dest_set, A_t const &a, V_t const &v) {
            if(dest_set->find(point_type(a, v)) != dest_set->end())
                return;
            insert_set = dest_set;
            set_element = insert_set->insert(point_type(a, v)).first;
            element_exist = true;
            Rollback = true;
        }

        // Stałe informują funkcje które maksimum jest wstawiane.
#define LEFT_INS -1
#define CENTER_INS 0
#define RIGHT_INS 1

        // Bezpieczne wstawianie nowego maksimum.
        void insert_to_set_max(set_max_t *dest_set, point_type const &point, int flag) {
            if(dest_set->find(point) != dest_set->end())
                return;
            insert_set_max = dest_set;
            switch (flag) {
                case LEFT_INS:
                    max_left = insert_set_max->insert(point).first;
                    left_exist = true;
                    break;
                case CENTER_INS:
                    max_center = insert_set_max->insert(point).first;
                    center_exist = true;
                    break;
                case RIGHT_INS:
                    max_right = insert_set_max->insert(point).first;
                    right_exist = true;
                    break;
            }
            Rollback = true;
        }

        // Odwołuje potrzebe rollbacka.
        void DropRollback() noexcept {
            Rollback = false;
        }

    };

public:
    // Implementacja punktu.
    class point_type {
    private:
        // Atrybuty
        A_t argument;
        V_t val;

        // Konstruktor parametrowy jest dostępny dla przyjaznych struktur.
        point_type(const A_t &a, const V_t &v) : argument(a), val(v) {}

    public:
        // Konstruktory
        point_type(const point_type &rhs) : argument(rhs.argument), val(rhs.val) {}

        point_type &operator=(const point_type &rhs) {
            return point_type(rhs);
        }

        // Getery
        A const &arg() const noexcept {
            return *argument;
        }

        V const &value() const noexcept {
            return *val;
        }

        // Klasa FunctionMaxima może korzystać z prywatnych rzeczy tej klasy.
        friend class FunctionMaxima;
    };

    // Publiczne aliasy iteratorów.
    using iterator = typename set_t::const_iterator;
    using mx_iterator = typename set_max_t::const_iterator;
    // Alias rozmiaru funkcji.
    using size_type = size_t;

    // Konstruktor bezparametrowy.
    FunctionMaxima() {}

    // Konstruktor kopiujący.
    FunctionMaxima(const FunctionMaxima &rhs) : function(rhs.function), points(rhs.points), maxima(rhs.maxima) {}

    // Operator przypisania.
    FunctionMaxima &operator=(const FunctionMaxima &rhs) {
        if (this == &rhs)
            return *this;

        map_t mapTemp(rhs.function);
        set_t pointsTemp(rhs.points);
        set_max_t maxTemp(rhs.maxima);

        mapTemp.swap(function);
        pointsTemp.swap(points);
        maxTemp.swap(maxima);
        return *this;
    }

    // Zwraca wartość funkcji dla podanego argumentu.
    V const &value_at(A const &a) const {
        auto found = function.find(MSA(a));
        if (found == function.end())
            throw InvalidArg();
        return *(found->second);
    }

    // Dodaje punkt do funkcji, a jeżeli istnieje już o takim argumencie to zmienia wartość.
    void set_value(A const &a, V const &v) {

        auto sh_a = MSA(a);
        auto sh_v = MSV(v);

        insertGuard guardian;

        auto found = function.find(sh_a);

        if (found != function.end()) {
            auto element = *found->second;
            if (!(v < element) && !(element < v))return;
        }

        guardian.insert_to_set(&points, sh_a, sh_v); // guard here
        auto current = points.find(point_type(sh_a, sh_v));

        auto prev = current;
        auto next = current;

        bool insertCurrent;
        auto prevBeforePrev = points.end();
        bool erasePrevBeforePrev = false;
        bool eraseNext = false;
        
        auto nextAfterNext = points.end();
        bool eraseNextAfterNext = false;
        bool erasePrev = false;

        bool insertPrev = false;
        bool insertNext = false;

        if (found == function.end()) {
            insertCurrent =  checkAddCenter(current, erasePrev, eraseNext);

            if (current != points.begin()) {
                --prev;
                insertPrev = checkAddCenter(prev, erasePrevBeforePrev, eraseNext);
                if(erasePrevBeforePrev){
                    prevBeforePrev = prev;
                    prevBeforePrev--;
                }
            }

            next++;

            if(next != points.end()){
                insertNext = checkAddCenter(next, erasePrev, eraseNextAfterNext);
                if(eraseNextAfterNext) {
                    nextAfterNext = next;
                    nextAfterNext++;
                }
            }

            set_iterator prev_max;
            set_iterator next_max;
            set_iterator prev_prev_max;
            set_iterator next_next_max;

            if(erasePrev)
                prev_max = maxima.find(*prev);
            if(eraseNext)
                next_max = maxima.find(*next);
            if(erasePrevBeforePrev)
                prev_prev_max = maxima.find(*prevBeforePrev);
            if(eraseNextAfterNext)
                next_next_max = maxima.find(*nextAfterNext);

            if(insertCurrent)
                guardian.insert_to_set_max(&maxima, *current, CENTER_INS);
            if (insertPrev)
                guardian.insert_to_set_max(&maxima, *prev, LEFT_INS);
            if (insertNext)
                guardian.insert_to_set_max(&maxima, *next, RIGHT_INS);


            function[sh_a] = sh_v;

            if (erasePrevBeforePrev)
                maxima.erase(prev_prev_max);
            if (eraseNextAfterNext)
                maxima.erase(next_next_max);
            if (eraseNext)
                maxima.erase(next_max);
            if (erasePrev)
                maxima.erase(prev_max);

        } else {
            bool nextAfterPrev = false;
            bool prevBeforeNext = false;
            auto sameArg = points.find(point_type{sh_a, found->second});
            auto temp = current;
            --temp;
            bool left = false;

            if (sameArg == temp) {
                left = true;
                insertCurrent = checkAddLeft(current, erasePrev, eraseNext);
            } else {
                insertCurrent = checkAddRight(current,  erasePrev, eraseNext);
            }

            auto sameArgMax = maxima.find(point_type(sameArg->argument, sameArg->val));

            if (left) {
                if (sameArg != points.begin()) {
                    --prev;
                    --prev;
                    insertPrev = checkAddRight(prev, erasePrevBeforePrev, nextAfterPrev);
                    if(erasePrevBeforePrev){
                        prevBeforePrev = prev;
                        prevBeforePrev--;
                    }
                }

                next++;
                if(next != points.end()){
                    insertNext = checkAddCenter(next, prevBeforeNext, eraseNextAfterNext);
                    if(eraseNextAfterNext) {
                        nextAfterNext = next;
                        nextAfterNext++;
                    }
                }

            } else {
                if (current != points.begin()) {
                    --prev;
                    insertPrev = checkAddCenter(prev, erasePrevBeforePrev, nextAfterPrev);
                    if(erasePrevBeforePrev){
                        prevBeforePrev = prev;
                        prevBeforePrev--;
                    }
                }

                next++;
                next++;
                if(next != points.end()){
                    insertNext = checkAddLeft(next, prevBeforeNext, eraseNextAfterNext);
                    if(eraseNextAfterNext) {
                        nextAfterNext = next;
                        nextAfterNext++;
                    }
                }
            }

            set_iterator prev_max;
            set_iterator next_max;
            set_iterator prev_prev_max;
            set_iterator next_next_max;

            if(erasePrev)
                prev_max = maxima.find(*prev);
            if(eraseNext)
                next_max = maxima.find(*next);
            if(erasePrevBeforePrev)
                prev_prev_max = maxima.find(*prevBeforePrev);
            if(eraseNextAfterNext)
                next_next_max = maxima.find(*nextAfterNext);

            if (insertCurrent)
                guardian.insert_to_set_max(&maxima, point_type{sh_a, sh_v}, CENTER_INS);
            if (insertPrev)
                guardian.insert_to_set_max(&maxima, *prev, LEFT_INS);
            if (insertNext)
                guardian.insert_to_set_max(&maxima, *next, RIGHT_INS);

            function[sh_a] = sh_v;

            if (erasePrevBeforePrev)
                maxima.erase(prev_prev_max);
            if (eraseNextAfterNext)
                maxima.erase(next_next_max);
            if (eraseNext)
                maxima.erase(next_max);
            if (erasePrev)
                maxima.erase(prev_max);
            if(sameArgMax != maxima.end())
                maxima.erase(sameArgMax);

            points.erase(sameArg);
        }
        guardian.DropRollback();

    }

    // Usuwa punkt z funkcji.
    void erase(A const &a) {
        insertGuard guardian;
        auto sh_a = MSA(a);
        auto found = function.find(sh_a);

        if (found != function.end()) {

            auto current = points.find(point_type(sh_a, found->second));
            auto prev = current;
            auto next = current;

            bool eraseNext = false;
            bool erasePrevBeforePrev = false;
            bool insertPrev = false;
            auto prevBeforePrev = points.end();

            if (current != points.begin()) {
                --prev;
                insertPrev = checkAddRight(prev, erasePrevBeforePrev, eraseNext);
                if(erasePrevBeforePrev){
                    prevBeforePrev = prev;
                    prevBeforePrev--;
                }
            }

            bool eraseNextAfterNext = false;
            bool erasePrev = false;
            bool insertNext = false;
            auto nextAfterNext = points.end();
            next++;

            if(next != points.end()){
                insertNext = checkAddLeft(next, erasePrev, eraseNextAfterNext);
                if(eraseNextAfterNext) {
                    nextAfterNext = next;
                    nextAfterNext++;
                }
            }

            set_iterator curr_max = maxima.find(*current);
            set_iterator prev_max;
            set_iterator next_max;
            set_iterator prev_prev_max;
            set_iterator next_next_max;

            if(erasePrev)
                prev_max = maxima.find(*prev);
            if(eraseNext)
                next_max = maxima.find(*next);
            if(erasePrevBeforePrev)
                prev_prev_max = maxima.find(*prevBeforePrev);
            if(eraseNextAfterNext)
                next_next_max = maxima.find(*nextAfterNext);

            if (insertPrev)
                guardian.insert_to_set_max(&maxima, *prev, LEFT_INS);
            if (insertNext)
                guardian.insert_to_set_max(&maxima, *next, RIGHT_INS);

            if (erasePrevBeforePrev)
                maxima.erase(prev_prev_max);
            if (eraseNextAfterNext)
                maxima.erase(next_next_max);
            if (eraseNext)
                maxima.erase(next_max);
            if (erasePrev)
                maxima.erase(prev_max);
            if(curr_max != maxima.end())
                maxima.erase(curr_max);

            points.erase(current);
            function.erase(found);
        }
        guardian.DropRollback();
    }

    // Zwraca iterator na pierwszy element w funkcji.
    iterator begin() const noexcept {
        return points.begin();
    }

    // Zwraca iterator na koniec funkcji.
    iterator end() const noexcept {
        return points.end();
    }

    // Zwraca iterator szukanego elementu lub end(), jeżeli nie istnieje punkt o takim argumencie.
    iterator find(A const &a) const {
        auto sh_a = MSA(a);
        auto found = function.find(sh_a);
        if (found == function.end())
            return points.end();
        return points.find(point_type(sh_a, found->second));
    }

    // Zwraca iterator na pierwsze maksimum w funkcji.
    mx_iterator mx_begin() const noexcept {
        return maxima.begin();
    }

    // Zwraca iterator na koniec maksimów funkcji.
    mx_iterator mx_end() const noexcept {
        return maxima.end();
    }

    // Zwraca liczbę punktów w funkcji.
    size_type size() const noexcept {
        return points.size();
    }
};

#endif //FUNCTION_MAXIMA_H
