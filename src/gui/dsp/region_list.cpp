#include "gui/dsp/audio_region.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/region_list.h"

RegionList::RegionList (QObject * parent) : QAbstractListModel (parent) { }

QHash<int, QByteArray>
RegionList::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[Qt::DisplayRole] = "region";
  return roles;
}

int
RegionList::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (regions_.size ());
}

QVariant
RegionList::data (const QModelIndex &index, int role) const
{
  if (!index.isValid () || index.row () >= static_cast<int> (regions_.size ()))
    return {};

  if (role == Qt::DisplayRole)
    {
      return QVariant::fromStdVariant (regions_[index.row ()]);
    }

  return {};
}

void
RegionList::init_after_cloning (
  const RegionList &other,
  ObjectCloneType   clone_type)
{
  beginResetModel ();
  regions_.clear ();
  regions_.reserve (other.regions_.size ());
  for (const auto region_var : other.regions_)
    {
      std::visit (
        [&] (auto &&other_region) {
          auto * clone = other_region->clone_qobject (this);
          regions_.push_back (clone);
        },
        region_var);
    }
  endResetModel ();
}

void
RegionList::clear ()
{
  beginResetModel ();
  regions_.clear ();
  endResetModel ();
}
