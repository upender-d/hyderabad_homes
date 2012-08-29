class AddPriceToUserLookingForProperties < ActiveRecord::Migration
  def change
    add_column :user_looking_for_properties, :price, :float
  end
end
