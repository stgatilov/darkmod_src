/***************************************************************************
 *
 * PROJECT: The Dark Mod
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 *
 ***************************************************************************/

#ifndef __SHOPITEM_H__
#define	__SHOPITEM_H__

#include <boost/shared_ptr.hpp>

// Represents an item for sale
class CShopItem
{
private:
	idStr		id;
	idStr		name;
	idStr		description;
	int			cost;
	idStr		image;
	int			count;
	bool		persistent;
	bool		canDrop;
	int			dropped;	// tels if dropped, store how many we had so player can undo drop

	// The list of entityDef names to add to the player's inventory 
	// when this shop item is purchased
	idStringList classNames;
	
	bool		stackable; // grayman (#2376)

public:
	CShopItem();

	CShopItem(const idStr& id, 
			  const idStr& name, 
			  const idStr& description, 
			  int cost,
			  const idStr& image, 
			  int count, 
			  bool persistent = false, 
			  bool canDrop = true,
			  bool stackable = false); // grayman (#2376)

	CShopItem(const CShopItem& item, 
			  int count, 
			  int cost = 0, 
			  bool persistent = false);

	// unique identifier for this item
	const idStr& GetID() const;

	// name of the item (for display)
	const idStr& GetName() const;

	// description of the item (for display)
	const idStr& GetDescription() const;

	// Get the list of classnames of entities to spawn for this shop item
	const idStringList& GetClassnames() const;

	// Adds a new classname for this shop item to be added to the player's inventory
	void AddClassname(const idStr& className);

	// cost of the item
	int GetCost();	

	// modal name (for displaying)
	const idStr& GetImage() const;

	// number of the items in this collection (number for sale,
	// or number user has bought, or number user started with)
	int GetCount();				

	// if starting item and it was dropped, the count before the drop (so we can undrop it)
	int GetDroppedCount();				

	// whether the item can be carried to the next mission
	bool GetPersistent();

	// whether the item can dropped by the player from the starting items list
	bool GetCanDrop();
	void SetCanDrop(bool canDrop);

	// grayman (#2376) - whether the item can be stacked
	bool GetStackable();
	void SetStackable(bool stackable);

	// modifies number of items
	void ChangeCount( int amount );

	// sets dropped => count and count => 0
	void Drop( void );

	// sets count => dropped and dropped => 0
	void Undrop( void );

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);
};
typedef boost::shared_ptr<CShopItem> CShopItemPtr;

// A list of shop items
typedef idList<CShopItemPtr> ShopItemList;

#endif
