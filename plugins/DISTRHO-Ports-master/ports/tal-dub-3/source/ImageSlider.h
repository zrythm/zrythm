/*
        ==============================================================================
        This file is part of Tal-Reverb by Patrick Kunz.

        Copyright(c) 2005-2010 Patrick Kunz, TAL
        Togu Audio Line, Inc.
        http://kunz.corrupt.ch

        This file may be licensed under the terms of of the
        GNU General Public License Version 2 (the ``GPL'').

        Software distributed under the License is distributed
        on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
        express or implied. See the GPL for the specific language
        governing rights and limitations.

        You should have received a copy of the GPL along with this
        program. If not, go to http://www.gnu.org/licenses/gpl.html
        or write to the Free Software Foundation, Inc.,
        51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
        ==============================================================================
 */

class ImageSlider : public Slider
{
public:

        // Curently only supports vertical sliders
        ImageSlider(const Image sliderImage, const int length, int index)
                : Slider(juce::String(index)),
                sliderImage(sliderImage),
                length(length)
        {
                setTextBoxStyle(NoTextBox, 0, 0, 0);
                setSliderStyle(LinearVertical);

                frameHeight = sliderImage.getHeight();
                frameWidth = sliderImage.getWidth();

                setRange(0.0f, 1.0f, 0.001f);

                this->setSliderSnapsToMousePosition(false);

        getProperties().set(Identifier("index"), index);
        }

        void paint(Graphics& g) override
        {
                double value = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
                g.drawImage(sliderImage, 0, (int)((1.0f - value) * length), frameWidth, frameHeight,
                        0, 0, frameWidth, frameHeight);
        }

private:
        const Image sliderImage;
        int length;
        int frameWidth, frameHeight;
};
