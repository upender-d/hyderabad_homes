class CreateUserProperties < ActiveRecord::Migration
  def change
    create_table :user_properties do |t|
      t.integer :user_id
      t.string :location
      t.integer :property_id
      t.boolean :is_current_location
      t.integer :ownership_type_id
      t.float :latitude
      t.float :longitude

      t.timestamps
    end
  end
end
