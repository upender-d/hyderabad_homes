class CreateUserLookingForProperties < ActiveRecord::Migration
  def change
    create_table :user_looking_for_properties do |t|
      t.integer :user_id
      t.string :location
      t.integer :property_id
      t.integer :looking_for_id

      t.float :latitude
      t.float :longitude

      t.timestamps
    end
  end
end
