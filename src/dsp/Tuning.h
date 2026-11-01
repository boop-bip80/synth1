#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace cs80
{
/** Microtuning support for the engine.

    A Tuning owns a MIDI-note -> frequency table. The default is standard
    12-tone equal temperament at A4 = 440 Hz, which is bit-compatible with the
    engine's previous closed-form pitch calculation.

    Two Scala file formats are understood:

      - `.scl` scale files (description, note count, cents or ratio intervals)
      - `.kbm` keyboard mappings (which MIDI key plays which scale degree)

    Parsing happens on the message thread. The resulting table is 128 doubles
    and is copied into the engine without any allocation. Nothing in the audio
    callback parses text or touches the heap.
*/
class Tuning
{
public:
    static constexpr int kNumNotes = 128;

    Tuning() { setEqualTemperament(); }

    /** Restores 12-TET and the default chromatic keyboard mapping. */
    void setEqualTemperament (double a4 = 440.0)
    {
        if (! (a4 > 0.0))
            a4 = 440.0;

        for (int note = 0; note < kNumNotes; ++note)
            table[(size_t) note] = a4 * std::exp2 (((double) note - 69.0) / 12.0);

        scaleCents = { 0.0, 100.0, 200.0, 300.0, 400.0, 500.0,
                       600.0, 700.0, 800.0, 900.0, 1000.0, 1100.0, 1200.0 };
        periodCents = 1200.0;
        description = "12-TET A440";
        equalTemperament = true;

        mapSize = 12;
        mapFirstNote = 0;
        mapLastNote = 127;
        mapMiddleNote = 60;
        mapReferenceNote = 69;
        mapReferenceFrequency = a4;
        mapOctaveDegree = 12;
        mapEntries.clear();

        for (int i = 0; i < 12; ++i)
            mapEntries.push_back (i);

        rebuild();
    }

    bool isEqualTemperament() const noexcept { return equalTemperament; }
    const std::string& name() const noexcept { return description; }
    int scaleNoteCount() const noexcept { return (int) scaleCents.size() - 1; }

    /** Frequency of an integer MIDI note. */
    double noteFrequency (int midiNote) const noexcept
    {
        const int note = std::clamp (midiNote, 0, kNumNotes - 1);
        return table[(size_t) note];
    }

    /** Frequency for a fractional note number. Interpolation happens in the
        log-frequency domain, so a default 12-TET table reproduces
        `440 * 2^((note - 69) / 12)` exactly, including glides. */
    double frequencyForNote (double midiNote) const noexcept
    {
        if (! (midiNote == midiNote)) // NaN guard
            return 440.0;

        const double clamped = std::clamp (midiNote, 0.0, (double) (kNumNotes - 1));
        const int lower = (int) std::floor (clamped);
        const int upper = std::min (lower + 1, kNumNotes - 1);
        const double fraction = clamped - (double) lower;

        const double f0 = table[(size_t) lower];
        const double f1 = table[(size_t) upper];

        if (fraction <= 0.0 || f1 <= 0.0 || f0 <= 0.0)
            return f0;

        const double log0 = std::log2 (f0);
        const double log1 = std::log2 (f1);
        return std::exp2 (log0 + (log1 - log0) * fraction);
    }

    //==========================================================================
    /** Parses a Scala `.scl` file. On success the tuning is replaced; on
        failure the previous tuning is left untouched and `error` explains why. */
    bool loadScala (const std::string& text, std::string* error = nullptr)
    {
        std::vector<std::string> lines = meaningfulLines (text);

        if (lines.size() < 2)
            return fail (error, "not a Scala scale: too few lines");

        const std::string newDescription = lines[0];
        int count = 0;

        if (! parseInt (lines[1], count) || count < 1 || count > 127)
            return fail (error, "not a Scala scale: invalid note count");

        if ((int) lines.size() < count + 2)
            return fail (error, "not a Scala scale: fewer intervals than declared");

        std::vector<double> cents;
        cents.reserve ((size_t) count + 1);
        cents.push_back (0.0);

        double total = 0.0;

        for (int i = 0; i < count; ++i)
        {
            double intervalCents = 0.0;

            if (! parseInterval (lines[(size_t) i + 2], intervalCents)
                || ! std::isfinite (intervalCents)
                || intervalCents <= 0.0
                || intervalCents > 100000.0)
                return fail (error, "not a Scala scale: invalid interval on line "
                                       + std::to_string (i + 3));

            total += intervalCents;
            cents.push_back (total);
        }

        if (! (total > 1.0) || ! std::isfinite (total))
            return fail (error, "not a Scala scale: empty octave");

        std::vector<double> newScale;
        newScale.reserve ((size_t) count + 1);

        // scala files may use a non-2/1 period. Normalise the period itself so
        // degree-to-note mapping stays monotonic, but keep every interval
        // relative to it instead of silently forcing an octave.
        for (double c : cents)
            newScale.push_back (c);

        // Replace only after everything parsed.
        scaleCents = std::move (newScale);
        periodCents = total;
        description = newDescription.empty() ? "Scala scale" : newDescription;
        equalTemperament = false;

        // A scale without an explicit keyboard mapping uses the scale's own
        // size as the map, anchored at A4 (as the engine always has).
        configureDefaultMap();
        rebuild();

        if (error != nullptr)
            error->clear();

        return true;
    }

    /** Parses a Scala `.kbm` mapping. It is applied to the current scale. */
    bool loadKeyboardMapping (const std::string& text, std::string* error = nullptr)
    {
        std::vector<std::string> lines = meaningfulLines (text);

        if (lines.size() < 7)
            return fail (error, "not a Scala mapping: too few lines");

        int values[7] {};

        for (int i = 0; i < 7; ++i)
            if (! parseInt (lines[(size_t) i], values[i]))
                return fail (error, "not a Scala mapping: invalid header value");

        const int newMapSize = values[0];
        const int newFirst = values[1];
        const int newLast = values[2];
        const int newOctaveDegree = values[6];

        if (newMapSize < 1 || newMapSize > 127
            || newFirst < 0 || newFirst > 127
            || newLast < newFirst || newLast > 127
            || values[4] < 0 || values[4] > 127
            || values[5] <= 0
            || newOctaveDegree < 1 || newOctaveDegree > scaleNoteCount())
            return fail (error, "not a Scala mapping: out-of-range header");

        // The last line is the mapping used for keys above `last note`.
        if ((int) lines.size() < 7 + newMapSize)
            return fail (error, "not a Scala mapping: fewer entries than map size");

        std::vector<int> newEntries;
        newEntries.reserve ((size_t) newMapSize);

        for (int i = 0; i < newMapSize; ++i)
        {
            int degree = 0;

            if (! parseInt (lines[(size_t) i + 7], degree))
                return fail (error, "not a Scala mapping: invalid entry");

            if (degree < -1 || degree >= newOctaveDegree)
                return fail (error, "not a Scala mapping: entry outside the octave");

            newEntries.push_back (degree);
        }

        mapSize = newMapSize;
        mapFirstNote = newFirst;
        mapLastNote = newLast;
        mapMiddleNote = values[3];
        mapReferenceNote = values[4];
        mapReferenceFrequency = (double) values[5];
        mapOctaveDegree = newOctaveDegree;
        mapEntries = std::move (newEntries);

        if (! (mapReferenceFrequency > 0.0))
            return fail (error, "not a Scala mapping: reference frequency must be positive");

        rebuild();

        if (error != nullptr)
            error->clear();

        return true;
    }

private:
    static bool fail (std::string* error, const std::string& message)
    {
        if (error != nullptr)
            *error = message;

        return false;
    }

    static std::string trim (const std::string& value)
    {
        std::size_t begin = 0, end = value.size();

        while (begin < end && std::isspace ((unsigned char) value[begin]))
            ++begin;

        while (end > begin && std::isspace ((unsigned char) value[end - 1]))
            --end;

        return value.substr (begin, end - begin);
    }

    static std::vector<std::string> meaningfulLines (const std::string& text)
    {
        std::vector<std::string> result;
        std::string current;

        for (char c : text)
        {
            if (c == '\n')
            {
                const std::string line = trim (current);
                current.clear();

                if (! line.empty() && line[0] != '!')
                    result.push_back (line);
            }
            else if (c != '\r')
            {
                current.push_back (c);
            }
        }

        const std::string line = trim (current);

        if (! line.empty() && line[0] != '!')
            result.push_back (line);

        return result;
    }

    static bool parseInt (const std::string& text, int& value)
    {
        try
        {
            std::size_t used = 0;
            const int parsed = std::stoi (trim (text), &used);

            if (used == 0)
                return false;

            value = parsed;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static bool parseDouble (const std::string& text, double& value)
    {
        try
        {
            std::size_t used = 0;
            const double parsed = std::stod (trim (text), &used);

            if (used == 0 || ! std::isfinite (parsed))
                return false;

            value = parsed;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    /** Scala intervals are either a ratio (`3/2`) or a value in cents. */
    static bool parseInterval (const std::string& line, double& cents)
    {
        double centsPerPeriod = 1200.0; // Scala's convention for .scl
        const std::string value = trim (line);

        if (value.empty())
            return false;

        const std::size_t slash = value.find ('/');

        if (slash == std::string::npos)
        {
            double raw = 0.0;

            if (! parseDouble (value, raw))
                return false;

            // A bare number is a value in cents.
            cents = raw;
            return true;
        }

        double numerator = 0.0, denominator = 0.0;

        if (! parseDouble (value.substr (0, slash), numerator)
            || ! parseDouble (value.substr (slash + 1), denominator)
            || numerator <= 0.0 || denominator <= 0.0)
            return false;

        cents = centsPerPeriod * std::log2 (numerator / denominator);
        return std::isfinite (cents);
    }

    /** Called after a new scale is loaded: use the scale size as the map and
        anchor middle C so a scale root lands where musicians expect it. In
        12-TET this is identical to the previous A4 = 440 calculation. */
    void configureDefaultMap()
    {
        mapSize = scaleNoteCount();
        mapFirstNote = 0;
        mapLastNote = 127;
        mapMiddleNote = 60;
        mapReferenceNote = 60;
        mapReferenceFrequency = 261.6255653005986; // C4 in 12-TET
        mapOctaveDegree = scaleNoteCount();
        mapEntries.clear();

        for (int i = 0; i < mapSize; ++i)
            mapEntries.push_back (i);
    }

    static int floorDiv (int a, int b)
    {
        const int q = a / b;
        return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
    }

    static int mod (int a, int b)
    {
        const int r = a % b;
        return r < 0 ? r + b : r;
    }

    /** Scale degree played by a MIDI note under the current mapping. */
    int degreeForNote (int note) const
    {
        if (mapEntries.empty() || mapSize <= 0)
            return note;

        const int size = std::max (1, mapSize);

        if (note < mapFirstNote)
        {
            // Extend the mapping downward one map at a time. Scala's own
            // reference implementation does the same.
            const int offset = floorDiv (note - mapFirstNote, size);
            const int index = mod (note - mapFirstNote, size);
            const int entry = mapEntries[(size_t) index];
            return entry + offset * mapOctaveDegree;
        }

        if (note <= mapLastNote)
        {
            const int index = note - mapFirstNote;
            const int entry = mapEntries[(size_t) (index % size)];
            return entry + floorDiv (index, size) * mapOctaveDegree;
        }

        const int index = note - mapFirstNote;
        const int entry = mapEntries[(size_t) (index % size)];
        return entry + floorDiv (index, size) * mapOctaveDegree;
    }

    double centsForDegree (int degree) const
    {
        const int size = std::max (1, (int) scaleCents.size() - 1);
        const int index = mod (degree, size);
        const int octave = floorDiv (degree, size);
        return scaleCents[(size_t) index] + (double) octave * periodCents;
    }

    void rebuild()
    {
        const double referenceCents = centsForDegree (degreeForNote (mapReferenceNote));
        const double referenceFrequency = mapReferenceFrequency > 0.0 ? mapReferenceFrequency : 440.0;

        for (int note = 0; note < kNumNotes; ++note)
        {
            const double cents = centsForDegree (degreeForNote (note));
            table[(size_t) note] = referenceFrequency
                                 * std::exp2 ((cents - referenceCents) / 1200.0);
        }
    }

    std::array<double, kNumNotes> table {};
    std::vector<double> scaleCents;
    double periodCents = 1200.0;
    std::string description = "12-TET A440";
    bool equalTemperament = true;

    int mapSize = 12;
    int mapFirstNote = 0;
    int mapLastNote = 127;
    int mapMiddleNote = 60;
    int mapReferenceNote = 69;
    double mapReferenceFrequency = 440.0;
    int mapOctaveDegree = 12;
    std::vector<int> mapEntries;
};
} // namespace cs80
