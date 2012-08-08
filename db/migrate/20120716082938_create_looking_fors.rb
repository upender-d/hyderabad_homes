class CreateLookingFors < ActiveRecord::Migration
  def change
    create_table :looking_fors do |t|
      t.string :name

      t.timestamps
    end
  end
end
