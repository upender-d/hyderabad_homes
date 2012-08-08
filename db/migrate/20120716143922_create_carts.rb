class CreateCarts < ActiveRecord::Migration
  def change
    create_table :carts do |t|
      t.string :property_type
      t.string :looking_for
      t.string :location

      t.timestamps
    end
  end
end
